# Verifica lo que dice el reporte de Argon: ejecutables, archivos por fase y tests.
# No borra ni modifica nada. Ejecutar como archivo.

$padre = "C:\Proyectos\NEW MOTOR"
$raiz = Join-Path $padre "ClockGears Engine"
$modulo = Join-Path $raiz "modules\clockgears"
$proyecto = Join-Path $raiz "test_project"
$exeConsola = Join-Path $raiz "ClockGears.console.exe"
$resumenTxt = Join-Path $raiz "diag_estado.txt"

if (-not (Test-Path $raiz)) { Write-Host "No existe $raiz" -ForegroundColor Red; exit 1 }

$salida = New-Object System.Collections.Generic.List[string]

$salida.Add("=== ESPACIO LIBRE EN C: ===")
$disco = Get-PSDrive -Name C
$salida.Add(("Libre: {0} GB" -f [Math]::Round($disco.Free / 1GB, 1)))

$salida.Add("")
$salida.Add("=== EJECUTABLES MAYORES DE 5 MB EN NEW MOTOR (debe haber solo los de ClockGears Engine\ y bin\) ===")
Get-ChildItem -Path $padre -Recurse -File -Force -Filter *.exe -ErrorAction SilentlyContinue | Where-Object { $_.Length -gt 5MB } | Sort-Object Length -Descending | ForEach-Object {
    $salida.Add(("{0,8:N1} MB  {1}  {2}" -f ($_.Length / 1MB), $_.LastWriteTime.ToString("dd/MM HH:mm"), $_.FullName))
}

$salida.Add("")
$salida.Add("=== ARCHIVOS DEL MODULO RELACIONADOS CON LAS FASES (nombre y líneas) ===")
$patron = "turn_battle|damage|time_manager|routine|follower|message|audio|cutscene|player|interpreter|pathfind|save_manager|simulator|spawn"
if (Test-Path $modulo) {
    $encontrados = Get-ChildItem -Path $modulo -Recurse -File -Include *.cpp,*.h -ErrorAction SilentlyContinue | Where-Object { $_.Name -match $patron }
    if ($encontrados) {
        foreach ($archivo in $encontrados) {
            $lineasArchivo = (Get-Content $archivo.FullName | Measure-Object -Line).Lines
            $salida.Add(("{0,6} lineas  {1}" -f $lineasArchivo, $archivo.FullName.Replace($raiz + "\", "")))
        }
    } else {
        $salida.Add("(ninguno)")
    }
} else {
    $salida.Add("No existe $modulo")
}

$salida.Add("")
$salida.Add("=== TESTS DE test_project (*.gd que empiezan con test_) ===")
if (-not (Test-Path $exeConsola)) { $exeConsola = Join-Path $raiz "bin\clockgears.windows.editor.x86_64.console.exe" }
$tests = Get-ChildItem -Path $proyecto -Filter "test_*.gd" -File -ErrorAction SilentlyContinue
if (-not $tests) { $salida.Add("(no hay tests)") }
foreach ($test in $tests) {
    Write-Host ("Ejecutando " + $test.Name + " (máximo 90 s)...")
    $salidaTest = Join-Path $raiz ("diag_" + $test.BaseName + "_out.txt")
    $errorTest = Join-Path $raiz ("diag_" + $test.BaseName + "_err.txt")
    $argumentos = '--headless --path "' + $proyecto + '" -s ' + $test.Name
    $proceso = Start-Process -FilePath $exeConsola -ArgumentList $argumentos -RedirectStandardOutput $salidaTest -RedirectStandardError $errorTest -PassThru -NoNewWindow
    $proceso | Wait-Process -Timeout 90 -ErrorAction SilentlyContinue
    $colgado = $false
    if (-not $proceso.HasExited) { $colgado = $true; Stop-Process -Id $proceso.Id -Force -ErrorAction SilentlyContinue }
    $lineasTest = @()
    if (Test-Path $salidaTest) { $lineasTest += Get-Content $salidaTest }
    if (Test-Path $errorTest) { $lineasTest += Get-Content $errorTest }
    $salida.Add("")
    $salida.Add("--- " + $test.Name + " ---")
    $salida.Add("Colgado (matado a los 90 s): " + $colgado)
    $salida.Add("PASS: " + @($lineasTest | Select-String -Pattern "PASS|EXITOSAMENTE").Count + "   FAIL: " + @($lineasTest | Select-String -Pattern "FAIL").Count + "   SCRIPT ERROR: " + @($lineasTest | Select-String -Pattern "SCRIPT ERROR").Count)
    $salida.Add("Últimas 12 líneas:")
    $lineasTest | Select-Object -Last 12 | ForEach-Object { $salida.Add($_) }
}

$salida | Out-File -Encoding utf8 $resumenTxt
Write-Host ""
Get-Content $resumenTxt
Write-Host ""
Write-Host "Guardado en: $resumenTxt" -ForegroundColor Green
