# make_icon.ps1 — 把 icon.png 转为 icon.ico，然后生成 .rc 编译进 exe

Add-Type -AssemblyName System.Drawing

$pngPath = "assets\pics\icon.png"
$icoPath = "assets\pics\icon.ico"

$bmp = [System.Drawing.Bitmap]::FromFile((Resolve-Path $pngPath))

# 写入 ICO 文件 (标准格式: ICO header + 1 entry + PNG data)
$ms = New-Object System.IO.MemoryStream
$bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
$pngBytes = $ms.ToArray()
$ms.Close()
$bmp.Dispose()

$fs = [System.IO.File]::OpenWrite($icoPath)
$bw = New-Object System.IO.BinaryWriter($fs)

# ICO header
$bw.Write([uint16]0)      # reserved
$bw.Write([uint16]1)      # ICO type
$bw.Write([uint16]1)      # image count

# Entry
$w = 256; if ($w -ge 256) { $w = 0 }  # 0 = 256px
$h = 256; if ($h -ge 256) { $h = 0 }
$bw.Write([byte]$w)
$bw.Write([byte]$h)
$bw.Write([byte]0)        # palette
$bw.Write([byte]0)        # reserved
$bw.Write([uint16]1)      # color planes
$bw.Write([uint16]32)     # bits per pixel
$bw.Write([uint32]$pngBytes.Length)  # data size
$bw.Write([uint32]22)     # offset (6 + 16)

# PNG data
$bw.Write($pngBytes)
$bw.Close()
$fs.Close()

Write-Host "Created: $icoPath ($($pngBytes.Length) bytes)"
