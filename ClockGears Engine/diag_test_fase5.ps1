# Corre el test de la Fase 5 con límite de 90 segundos (si se cuelga, lo mata) y resume el resultado.
# Ejecutar como archivo. Cierra antes cualquier tarea colgada del test en Antigravity.

$raiz = "C:\Proyectos\NEW MOTOR\ClockGears Engine"
$exe = Join-Path $raiz "ClockGears.console.exe"
$proyecto = Join-Path $raiz "test_project"
$scriptTest = Join-Path $proyecto "test_fase5_database.gd"
$salidaTxt = Join-Path $raiz "diag_test_fase5_stdout.txt"
$errorTxt = Join-Path $raiz "diag_test_fase5_stderr.txt"
$resumenTxt = Join-Path $raiz "diag_test_fase5.txt"

if (-not (Test-Path $exe)) { $exe = Join-Path $raiz "bin\clockgears.windows.editor.x86_64.console.exe" }
if (-not (Test-Path $exe)) { Write-Host "No encuentro ClockGears.console.exe" -ForegroundColor Red; exit 1 }
if (-not (Test-Path $scriptTest)) { Write-Host "No existe $scriptTest" -ForegroundColor Red; exit 1 }

Write-Host "Ejecutando el test (máximo 90 segundos)..."
$argumentos = '--headless --path "' + $proyecto + '" -s test_fase5_database.gd'
$proceso = Start-Process -FilePath $exe -ArgumentList $argumentos -RedirectStandardOutput $salidaTxt -RedirectStandardError $errorTxt -PassThru -NoNewWindow
$proceso | Wait-Process -Timeout 90 -ErrorAction SilentlyContinue

$colgado = $false
if (-not $proceso.HasExited) {
    $colgado = $true
    Stop-Process -Id $proceso.Id -Force -ErrorAction SilentlyContinue
}

$lineas = @()
if (Test-Path $salidaTxt) { $lineas += Get-Content $salidaTxt }
if (Test-Path $errorTxt) { $lineas += Get-Content $errorTxt }

$resumen = New-Object System.Collections.Generic.List[string]
$resumen.Add("=== RESULTADO ===")
$resumen.Add("Se quedó colgado (lo maté a los 90 s): " + $colgado)
$resumen.Add("Líneas [PASS]: " + @($lineas | Select-String -Pattern "\[PASS\]").Count)
$resumen.Add("Líneas [FAIL]: " + @($lineas | Select-String -Pattern "\[FAIL\]").Count)
$resumen.Add("Líneas SCRIPT ERROR: " + @($lineas | Select-String -Pattern "SCRIPT ERROR").Count)
$resumen.Add("Mensaje final 'ALL ... PASSED' presente: " + (@($lineas | Select-String -Pattern "PASSED 100%").Count -gt 0))
$resumen.Add("")
$resumen.Add("=== SALIDA COMPLETA ===")
foreach ($linea in $lineas) { $resumen.Add($linea) }

$resumen | Out-File -Encoding utf8 $resumenTxt
Write-Host ""
Get-Content $resumenTxt
Write-Host ""
Write-Host "Guardado en: $resumenTxt" -ForegroundColor Green
