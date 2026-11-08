# Lee el log ya generado (diag_log_completo.txt) y busca de dónde salen los errores.
# No vuelve a ejecutar el motor.

$raiz = "C:\Proyectos\NEW MOTOR\ClockGears Engine"
$log = Join-Path $raiz "diag_log_completo.txt"
$salidaArchivo = Join-Path $raiz "diag_errores.txt"

if (-not (Test-Path $log)) {
    Write-Host "No existe $log. Corre primero diag_clockgears.ps1" -ForegroundColor Red
    exit 1
}

$lineas = Get-Content $log
$salida = New-Object System.Collections.Generic.List[string]

$salida.Add("=== ORIGEN DE 'Cannot get path of node' (linea 'at:' que sigue al error) ===")
$origenes = New-Object System.Collections.Generic.List[string]
for ($i = 0; $i -lt $lineas.Count; $i++) {
    if ($lineas[$i] -match "Cannot get path of node") {
        $sig = ""
        if ($i + 1 -lt $lineas.Count) { $sig = $lineas[$i + 1].Trim() }
        $origenes.Add($sig)
    }
}
$salida.Add("Total de errores: " + $origenes.Count)
$origenes | Group-Object | Sort-Object Count -Descending | ForEach-Object { $salida.Add(("{0,5}  {1}" -f $_.Count, $_.Name)) }

$salida.Add("")
$salida.Add("=== CONTEXTO DEL PRIMER ERROR (5 lineas antes y 3 despues) ===")
$primero = -1
for ($i = 0; $i -lt $lineas.Count; $i++) {
    if ($lineas[$i] -match "Cannot get path of node") { $primero = $i; break }
}
if ($primero -ge 0) {
    $desde = [Math]::Max(0, $primero - 5)
    $hasta = [Math]::Min($lineas.Count - 1, $primero + 3)
    for ($j = $desde; $j -le $hasta; $j++) { $salida.Add($lineas[$j]) }
}

$salida.Add("")
$salida.Add("=== OTROS ERRORES (sin fugas ni 'Cannot get path') CON SU LINEA 'at:' ===")
for ($i = 0; $i -lt $lineas.Count; $i++) {
    if ($lineas[$i] -match "^ERROR" -and $lineas[$i] -notmatch "leaked" -and $lineas[$i] -notmatch "Cannot get path") {
        $salida.Add($lineas[$i])
        if ($i + 1 -lt $lineas.Count) { $salida.Add($lineas[$i + 1]) }
    }
}

$salida | Out-File -Encoding utf8 $salidaArchivo
Write-Host ""
Get-Content $salidaArchivo
Write-Host ""
Write-Host "Guardado en: $salidaArchivo" -ForegroundColor Green
