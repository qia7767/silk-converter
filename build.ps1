$ErrorActionPreference = "Stop"
Push-Location $PSScriptRoot
try {
  if (!(Test-Path ffmpeg.exe)) { throw "Place FFmpeg 9.1 Essentials ffmpeg.exe in the project directory; see README.md." }
  $sources = Get-ChildItem vendor/silk/src/*.c | ForEach-Object { $_.FullName }
  & gcc -O2 -I vendor/silk/interface -I vendor/silk/src $sources vendor/silk/test/Decoder.c -o decoder.exe
  if ($LASTEXITCODE -ne 0) { throw "decoder build failed" }
  Add-Type -AssemblyName System.IO.Compression
  foreach ($name in @('decoder','ffmpeg')) {
    $inputFile = [System.IO.File]::OpenRead("$PSScriptRoot\$name.exe")
    $outFile = [System.IO.File]::Create("$PSScriptRoot\$name.gz")
    try { $gz = New-Object System.IO.Compression.GZipStream($outFile, [System.IO.Compression.CompressionMode]::Compress); try { $inputFile.CopyTo($gz) } finally { $gz.Dispose() } } finally { $inputFile.Dispose(); $outFile.Dispose() }
  }
  & "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe" /nologo /target:winexe /out:SilkConverter.exe /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /resource:decoder.gz,decoder.gz /resource:ffmpeg.gz,ffmpeg.gz Program.cs
  if ($LASTEXITCODE -ne 0) { throw "C# build failed" }
} finally { Pop-Location }
