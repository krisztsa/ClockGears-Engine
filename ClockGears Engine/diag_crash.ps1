# Captura por qué se cierra el motor: log, código de salida, backtrace y eventos de Windows.
# Ejecutar como archivo desde la carpeta del motor.

$raiz = "C:\Proyectos\NEW MOTOR\ClockGears Engine"
$exe = Join-Path $raiz "bin\clockgears.windows.editor.x86_64.exe"
$logCrash = Join-Path $raiz "diag_crash_log.txt"
$resumenCrash = Join-Path $raiz "diag_crash.txt"

if (-not (Test-Path $exe)) {
    Write-Host "No existe el exe: $exe" -ForegroundColor Red
    exit 1
}

Set-Location $raiz
$inicio = Get-Date

Write-Host ""
Write-Host "Se va a abrir el editor con --verbose." -ForegroundColor Yellow
Write-Host "Haz lo que hacías cuando se cerraba (abrir Base de Datos, pulsar algo, etc.)."
Write-Host "Cuando el motor se cierre, vuelve aquí y responde la pregunta."
Write-Host ""

& $exe --path test_project --editor --verbose 2>&1 | Out-File -Encoding utf8 $logCrash
$codigoSalida = $LASTEXITCODE

$lineas = Get-Content $logCrash
$salida = New-Object System.Collections.Generic.List[string]

$salida.Add("=== CODIGO DE SALIDA ===")
$salida.Add("Codigo: $codigoSalida  (0 = cierre normal; otro valor = fallo)")

$salida.Add("")
$salida.Add("=== BACKTRACE / CRASH HANDLER DE GODOT ===")
$crash = $lineas | Select-String -Pattern "handle_crash|Program crashed|backtrace|CrashHandler|Dumping the backtrace|\[\d+\] "
if ($crash) { $crash | Select-Object -First 60 | ForEach-Object { $salida.Add($_.Line) } } else { $salida.Add("(no hay backtrace en el log)") }

$salida.Add("")
$salida.Add("=== ULTIMAS 60 LINEAS DEL LOG (lo ultimo que hizo antes de cerrarse) ===")
$lineas | Select-Object -Last 60 | ForEach-Object { $salida.Add($_) }

$salida.Add("")
$salida.Add("=== ERRORES Y WARNINGS (sin los de p_image), CON SU LINEA 'at:' ===")
for ($i = 0; $i -lt $lineas.Count; $i++) {
    if ($lineas[$i] -match "^(ERROR|WARNING)" -and $lineas[$i] -notmatch "p_image.is_null" -and $lineas[$i] -notmatch "leaked") {
        $salida.Add($lineas[$i])
        if ($i + 1 -lt $lineas.Count -and $lineas[$i + 1] -match "^\s+at:") { $salida.Add($lineas[$i + 1]) }
    }
}

$salida.Add("")
$salida.Add("=== EVENTOS DE WINDOWS (Application Error desde que abriste el motor) ===")
try {
    $eventos = Get-WinEvent -FilterHashtable @{ LogName = "Application"; ProviderName = "Application Error"; StartTime = $inicio } -ErrorAction Stop
    foreach ($ev in $eventos) {
        $salida.Add($ev.TimeCreated.ToString("HH:mm:ss"))
        $salida.Add($ev.Message)
        $salida.Add("---")
    }
} catch {
    $salida.Add("(sin eventos de Application Error en ese periodo)")
}

Write-Host ""
$queHacias = Read-Host "Que estabas haciendo justo cuando se cerro? (escribe y Enter)"
$salida.Add("")
$salida.Add("=== LO QUE ESTABA HACIENDO EL USUARIO ===")
$salida.Add($queHacias)

$salida | Out-File -Encoding utf8 $resumenCrash
Write-Host ""
Get-Content $resumenCrash
Write-Host ""
Write-Host "Guardado en: $resumenCrash" -ForegroundColor Green
