#include "clockgears_choice_utils.h"

bool ClockGearsChoiceUtils::is_choices_start(const Dictionary &p_cmd) {
	String type = p_cmd.get("type", "");
	int code = int(p_cmd.get("code", 0));
	return type == "choices" || code == CMD_SHOW_CHOICES;
}

bool ClockGearsChoiceUtils::is_when_choice(const Dictionary &p_cmd) {
	String type = p_cmd.get("type", "");
	int code = int(p_cmd.get("code", 0));
	return type == "when_choice" || code == CG_CMD_WHEN_CHOICE;
}

bool ClockGearsChoiceUtils::is_when_cancel(const Dictionary &p_cmd) {
	String type = p_cmd.get("type", "");
	int code = int(p_cmd.get("code", 0));
	return type == "when_cancel" || code == CG_CMD_WHEN_CANCEL;
}

bool ClockGearsChoiceUtils::is_branch_end(const Dictionary &p_cmd) {
	String type = p_cmd.get("type", "");
	int code = int(p_cmd.get("code", 0));
	return type == "branch_end_choice" || code == CG_CMD_BRANCH_END_CHOICE;
}

int ClockGearsChoiceUtils::find_block_end(const Array &p_cmds, int p_start_idx) {
	if (p_start_idx < 0 || p_start_idx >= p_cmds.size()) {
		return -1;
	}
	int depth = 0;
	for (int i = p_start_idx; i < p_cmds.size(); i++) {
		Dictionary d = p_cmds[i];
		if (is_choices_start(d)) {
			depth++;
		} else if (is_branch_end(d)) {
			depth--;
			if (depth == 0) {
				return i;
			}
		}
	}
	return -1;
}

int ClockGearsChoiceUtils::find_block_start(const Array &p_cmds, int p_end_or_inside_idx) {
	if (p_end_or_inside_idx < 0 || p_end_or_inside_idx >= p_cmds.size()) {
		return -1;
	}
	int depth = 0;
	for (int i = p_end_or_inside_idx; i >= 0; i--) {
		Dictionary d = p_cmds[i];
		if (is_branch_end(d)) {
			depth++;
		} else if (is_choices_start(d)) {
			if (depth <= 1) {
				return i;
			}
			depth--;
		}
	}
	return -1;
}

int ClockGearsChoiceUtils::get_command_depth(const Array &p_cmds, int p_idx) {
	int depth = 0;
	int limit = CLAMP(p_idx, 0, p_cmds.size());
	for (int i = 0; i < limit; i++) {
		Dictionary d = p_cmds[i];
		if (is_choices_start(d)) {
			depth++;
		} else if (is_branch_end(d)) {
			if (depth > 0) depth--;
		}
	}
	return depth;
}

bool ClockGearsChoiceUtils::block_has_content(const Array &p_cmds, int p_start_idx) {
	int end_idx = find_block_end(p_cmds, p_start_idx);
	if (end_idx < 0) {
		return false;
	}
	for (int i = p_start_idx + 1; i < end_idx; i++) {
		Dictionary d = p_cmds[i];
		if (!is_when_choice(d) && !is_when_cancel(d)) {
			return true;
		}
	}
	return false;
}

HashMap<int, Array> ClockGearsChoiceUtils::extract_branches(const Array &p_cmds, int p_start_idx) {
	HashMap<int, Array> branches;
	int end_idx = find_block_end(p_cmds, p_start_idx);
	if (end_idx < 0) {
		return branches;
	}

	int current_branch = -999;
	int nested_depth = 0;

	for (int i = p_start_idx + 1; i < end_idx; i++) {
		Dictionary d = p_cmds[i];
		if (nested_depth == 0) {
			if (is_when_choice(d)) {
				current_branch = int(d.get("choice", 0));
				if (!branches.has(current_branch)) {
					branches[current_branch] = Array();
				}
				continue;
			} else if (is_when_cancel(d)) {
				current_branch = -1;
				if (!branches.has(current_branch)) {
					branches[current_branch] = Array();
				}
				continue;
			}
		}

		if (is_choices_start(d)) {
			nested_depth++;
		} else if (is_branch_end(d)) {
			if (nested_depth > 0) nested_depth--;
		}

		if (current_branch != -999) {
			if (!branches.has(current_branch)) {
				branches[current_branch] = Array();
			}
			branches[current_branch].push_back(d);
		}
	}

	return branches;
}

Array ClockGearsChoiceUtils::rebuild_choice_block(const Dictionary &p_choice_cmd, const HashMap<int, Array> &p_branches) {
	Array result;
	result.push_back(p_choice_cmd);

	int cancel_opt = int(p_choice_cmd.get("cancel_opt", p_choice_cmd.get("cancel_type", 0)));

	Vector<String> opts;
	for (int c = 1; c <= 6; c++) {
		String key = "c" + itos(c);
		String txt = p_choice_cmd.get(key, "");
		if (!txt.strip_edges().is_empty()) {
			opts.push_back(txt);
		}
	}

	for (int i = 0; i < opts.size(); i++) {
		Dictionary b;
		b["type"] = "when_choice";
		b["code"] = CG_CMD_WHEN_CHOICE;
		b["choice"] = i;
		b["text"] = opts[i];
		result.push_back(b);

		if (p_branches.has(i)) {
			Array b_cmds = p_branches[i];
			for (int j = 0; j < b_cmds.size(); j++) {
				result.push_back(b_cmds[j]);
			}
		}
	}

	if (cancel_opt == -1) {
		Dictionary bc;
		bc["type"] = "when_cancel";
		bc["code"] = CG_CMD_WHEN_CANCEL;
		bc["choice"] = -1;
		result.push_back(bc);

		if (p_branches.has(-1)) {
			Array c_cmds = p_branches[-1];
			for (int j = 0; j < c_cmds.size(); j++) {
				result.push_back(c_cmds[j]);
			}
		}
	}

	Dictionary bend;
	bend["type"] = "branch_end_choice";
	bend["code"] = CG_CMD_BRANCH_END_CHOICE;
	result.push_back(bend);

	return result;
}

bool ClockGearsChoiceUtils::can_move_up(const Array &p_cmds, int p_idx) {
	if (p_idx <= 0 || p_idx >= p_cmds.size()) {
		return false;
	}
	Dictionary cur = p_cmds[p_idx];
	if (is_when_choice(cur) || is_when_cancel(cur) || is_branch_end(cur)) {
		return false;
	}

	Dictionary prev = p_cmds[p_idx - 1];
	if (is_when_choice(prev) || is_when_cancel(prev)) {
		// Ya está al tope de su rama
		return false;
	}
	return true;
}

int ClockGearsChoiceUtils::move_up(Array &r_cmds, int p_idx) {
	if (!can_move_up(r_cmds, p_idx)) {
		return p_idx;
	}
	Dictionary cur = r_cmds[p_idx];

	if (is_choices_start(cur)) {
		int end_idx = find_block_end(r_cmds, p_idx);
		if (end_idx < 0) return p_idx;
		int len = end_idx - p_idx + 1;
		Array block;
		for (int i = 0; i < len; i++) {
			block.push_back(r_cmds[p_idx + i]);
		}

		Dictionary prev = r_cmds[p_idx - 1];
		int target_insert_idx = p_idx - 1;
		if (is_branch_end(prev)) {
			int prev_start = find_block_start(r_cmds, p_idx - 1);
			if (prev_start >= 0) {
				target_insert_idx = prev_start;
			}
		}

		for (int i = 0; i < len; i++) {
			r_cmds.remove_at(p_idx);
		}
		for (int i = 0; i < len; i++) {
			r_cmds.insert(target_insert_idx + i, block[i]);
		}
		return target_insert_idx;
	} else {
		Dictionary prev = r_cmds[p_idx - 1];
		if (is_branch_end(prev)) {
			int prev_start = find_block_start(r_cmds, p_idx - 1);
			if (prev_start >= 0) {
				Variant item = r_cmds[p_idx];
				r_cmds.remove_at(p_idx);
				r_cmds.insert(prev_start, item);
				return prev_start;
			}
		}
		Variant tmp = r_cmds[p_idx];
		r_cmds[p_idx] = r_cmds[p_idx - 1];
		r_cmds[p_idx - 1] = tmp;
		return p_idx - 1;
	}
}

bool ClockGearsChoiceUtils::can_move_down(const Array &p_cmds, int p_idx) {
	if (p_idx < 0 || p_idx >= p_cmds.size() - 1) {
		return false;
	}
	Dictionary cur = p_cmds[p_idx];
	if (is_when_choice(cur) || is_when_cancel(cur) || is_branch_end(cur)) {
		return false;
	}

	if (is_choices_start(cur)) {
		int end_idx = find_block_end(p_cmds, p_idx);
		if (end_idx < 0 || end_idx >= p_cmds.size() - 1) {
			return false;
		}
		Dictionary next = p_cmds[end_idx + 1];
		if (is_when_choice(next) || is_when_cancel(next) || is_branch_end(next)) {
			return false;
		}
		return true;
	} else {
		Dictionary next = p_cmds[p_idx + 1];
		if (is_when_choice(next) || is_when_cancel(next) || is_branch_end(next)) {
			return false;
		}
		return true;
	}
}

int ClockGearsChoiceUtils::move_down(Array &r_cmds, int p_idx) {
	if (!can_move_down(r_cmds, p_idx)) {
		return p_idx;
	}
	Dictionary cur = r_cmds[p_idx];

	if (is_choices_start(cur)) {
		int end_idx = find_block_end(r_cmds, p_idx);
		if (end_idx < 0) return p_idx;
		int len = end_idx - p_idx + 1;
		Array block;
		for (int i = 0; i < len; i++) {
			block.push_back(r_cmds[p_idx + i]);
		}

		Dictionary next = r_cmds[end_idx + 1];
		int target_pos = end_idx + 1;
		if (is_choices_start(next)) {
			int next_end = find_block_end(r_cmds, end_idx + 1);
			if (next_end >= 0) {
				target_pos = next_end;
			}
		}

		for (int i = 0; i < len; i++) {
			r_cmds.remove_at(p_idx);
		}
		int insert_pos = target_pos - len + 1;
		for (int i = 0; i < len; i++) {
			r_cmds.insert(insert_pos + i, block[i]);
		}
		return insert_pos;
	} else {
		Dictionary next = r_cmds[p_idx + 1];
		if (is_choices_start(next)) {
			int next_end = find_block_end(r_cmds, p_idx + 1);
			if (next_end >= 0) {
				Variant item = r_cmds[p_idx];
				r_cmds.remove_at(p_idx);
				r_cmds.insert(next_end, item);
				return next_end;
			}
		}
		Variant tmp = r_cmds[p_idx];
		r_cmds[p_idx] = r_cmds[p_idx + 1];
		r_cmds[p_idx + 1] = tmp;
		return p_idx + 1;
	}
}

void ClockGearsChoiceUtils::delete_choice_block(Array &r_cmds, int p_start_idx) {
	int end_idx = find_block_end(r_cmds, p_start_idx);
	if (end_idx < 0) {
		r_cmds.remove_at(p_start_idx);
		return;
	}
	int count = end_idx - p_start_idx + 1;
	for (int i = 0; i < count; i++) {
		r_cmds.remove_at(p_start_idx);
	}
}
