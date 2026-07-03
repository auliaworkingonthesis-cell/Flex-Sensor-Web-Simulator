# PowerShell script to generate ESP Web Tools manifests for all 9 sketches
# Save as: D:\Data_Lokal\Kuliah\Aulia Shabrina\Flex_Sensor_WebSim\tools\generate_manifests.ps1

$projectRoot = "D:\Data_Lokal\Kuliah\Aulia Shabrina\Flex_Sensor_WebSim"
$manifestsDir = Join-Path $projectRoot "manifests"

if (!(Test-Path $manifestsDir)) {
    New-Item -ItemType Directory -Path $manifestsDir -Force
}

$firmwares = @(
    @{ id = "l1_serial"; name = "Labsheet 1: Value Flex di Serial Monitor" },
    @{ id = "l1_lcd"; name = "Labsheet 1: Value Flex + Sudut + Tegangan di LCD" },
    @{ id = "l2_led"; name = "Labsheet 2: Flex + LED" },
    @{ id = "l2_servo"; name = "Labsheet 2: Flex + Servo" },
    @{ id = "l2_servo_led"; name = "Labsheet 2: Flex + Servo + Lampu" },
    @{ id = "l2_servo_led_lcd"; name = "Labsheet 2: Flex + Servo + Lampu + LCD" },
    @{ id = "l3_serial_servo"; name = "Labsheet 3: Flex + Serial + Servo" },
    @{ id = "l3_wifi_servo"; name = "Labsheet 3: Flex + Wifi + Servo" },
    @{ id = "l3_dual_servo"; name = "Labsheet 3: Flex + Serial + Wifi + Servo" }
)

foreach ($f in $firmwares) {
    $manifestPath = Join-Path $manifestsDir "$($f.id).json"
    $jsonContent = @"
{
  "name": "$($f.name)",
  "version": "1.0.0",
  "new_install_prompt_erase": false,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        { "path": "../binaries/$($f.id).bin", "offset": 65536 }
      ]
    }
  ]
}
"@
    Set-Content -Path $manifestPath -Value $jsonContent -Force
    Write-Output "Created manifest: $manifestPath"
}
