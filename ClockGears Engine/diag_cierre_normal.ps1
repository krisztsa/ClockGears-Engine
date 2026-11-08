# Abre el editor de forma normal (sin --quit-after). Tú lo usas un momento y lo cierras con la X.
# Al cerrar, el script resume los errores y fugas que salieron.

$raiz = "C:\Proyectos\NEW MOTOR\ClockGears Engine"
$exe = Join-Path $raiz "bin\clockgears.windows.editor.x86_64.exe"
$logNormal = Join-Path $raiz "diag_cierre_normal_log.txt"
$resumenNormal = Join-Path $raiz "diag_cierre_normal.txt"

if (-not (Test-Path $exe)) {
    Write-Host "No existe el exe: $exe" -ForegroundColor Red
    exit 1
}

Set-Location $raiz
Write-Host ""
Write-Host "Se va a abrir el editor. Haz esto:" -ForegroundColor Yellow
Write-Host "  1. Abre Base de Datos."
Write-Host "  2. Pulsa + en Actores (si existe) e intenta escribir un nombre."
Write-Host "  3. Cierra la Base de Datos."
Write-Host "  4. Cierra el editor con la X (cierre normal)."
Write-Host ""

& $exe --path test_project --editor 2>&1 | Out-File -Encoding utf8 $logNormal

$lineas = Get-Content $logNormal
$salida = New-Object System.Collections.Generic.List[string]

$salida.Add("=== CONTEOS ===")
$salida.Add("Lineas con 'leaked': " + @($lineas | Select-String -Pattern "leaked").Count)
$salida.Add("Errores 'Cannot get path of node': " + @($lineas | Select-String -Pattern "Cannot get path of node").Count)
$salida.Add("Lineas ERROR en total: " + @($lineas | Select-String -Pattern "^ERROR").Count)

$salida.Add("")
$salida.Add("=== ERRORES / WARNINGS (sin fugas), CON SU LINEA 'at:' ===")
for ($i = 0; $i -lt $lineas.Count; $i++) {
    if ($lineas[$i] -match "^(ERROR|WARNING)" -and $lineas[$i] -notmatch "leaked" -and $lineas[$i] -notmatch "Cannot get path") {
        $salida.Add($lineas[$i])
        if ($i + 1 -lt $lineas.Count -and $lineas[$i + 1] -match "^\s+at:") { $salida.Add($lineas[$i + 1]) }
    }
}

$salida.Add("")
$salida.Add("=== LINEAS DE FUGAS ===")
$lineas | Select-String -Pattern "leaked" | ForEach-Object { $salida.Add($_.Line) }

$salida | Out-File -Encoding utf8 $resumenNormal
Write-Host ""
Get-Content $resumenNormal
Write-Host ""
Write-Host "Guardado en: $resumenNormal" -ForegroundColor Green
