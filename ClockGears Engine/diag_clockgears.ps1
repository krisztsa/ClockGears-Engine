# Diagnóstico ClockGears: self-tests y fugas al salir
# Ejecutar como archivo desde la carpeta del motor.

$raiz = "C:\Proyectos\NEW MOTOR\ClockGears Engine"
$exe = Join-Path $raiz "bin\clockgears.windows.editor.x86_64.exe"
$logCompleto = Join-Path $raiz "diag_log_completo.txt"
$logResumen = Join-Path $raiz "diag_resumen.txt"

if (-not (Test-Path $exe)) {
    Write-Host "No existe el exe: $exe" -ForegroundColor Red
    exit 1
}

Set-Location $raiz
Write-Host "Ejecutando el motor con --verbose (tarda unos segundos)..."
& $exe --path test_project --editor --quit-after 1 --verbose 2>&1 | Out-File -Encoding utf8 $logCompleto

$lineas = Get-Content $logCompleto
$salida = New-Object System.Collections.Generic.List[string]

$salida.Add("=== LINEAS CON SelfTest / ClockGears ===")
$lineas | Select-String -Pattern "SelfTest|ClockGears" | Select-Object -First 60 | ForEach-Object { $salida.Add($_.Line) }

$salida.Add("")
$salida.Add("=== ERRORES Y WARNINGS (sin los de fugas) ===")
$lineas | Select-String -Pattern "^(ERROR|WARNING)" | Where-Object { $_.Line -notmatch "leaked" } | ForEach-Object { $salida.Add($_.Line) }

$salida.Add("")
$salida.Add("=== CLASES FILTRADAS EN ObjectDB (top 25) ===")
$fugas = $lineas | Select-String -Pattern "Leaked instance: (\w+):" | ForEach-Object { $_.Matches[0].Groups[1].Value }
if ($fugas) {
    $fugas | Group-Object | Sort-Object Count -Descending | Select-Object -First 25 | ForEach-Object { $salida.Add(("{0,5}  {1}" -f $_.Count, $_.Name)) }
} else {
    $salida.Add("(no se encontraron lineas 'Leaked instance' en el log)")
}

$salida.Add("")
$salida.Add("=== CLASES CON 'ClockGears' EN LAS FUGAS ===")
$cg = $lineas | Select-String -Pattern "Leaked instance: ClockGears\w+" | Select-Object -First 40
if ($cg) { $cg | ForEach-Object { $salida.Add($_.Line) } } else { $salida.Add("(ninguna)") }

$salida | Out-File -Encoding utf8 $logResumen
Write-Host ""
Write-Host "Listo. Pega el contenido de este archivo:" -ForegroundColor Green
Write-Host $logResumen
Write-Host ""
Get-Content $logResumen
