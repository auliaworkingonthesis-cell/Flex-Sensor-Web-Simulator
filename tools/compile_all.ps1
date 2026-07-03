# PowerShell script to compile all 9 practical sketches into ESP32 binaries (.bin)
# Save as: D:\Data_Lokal\Kuliah\Aulia Shabrina\Flex_Sensor_WebSim\tools\compile_all.ps1

$projectRoot = "D:\Data_Lokal\Kuliah\Aulia Shabrina\Flex_Sensor_WebSim"
$firmwareDir = Join-Path $projectRoot "firmware"
$srcFile     = Join-Path $firmwareDir "src\main.cpp"
$backupFile  = Join-Path $firmwareDir "src\main.cpp.bak"
$binariesDir = Join-Path $projectRoot "binaries"

# Create binaries dir if not exists
if (!(Test-Path $binariesDir)) {
    New-Item -ItemType Directory -Path $binariesDir -Force
}

# Backup main.cpp
if (Test-Path $srcFile) {
    Copy-Item $srcFile $backupFile -Force
    Write-Output "Backed up main.cpp"
} else {
    Write-Error "main.cpp not found. Cannot proceed."
    exit 1
}

# List of sketches to compile
$sketches = @(
    @{
        id = "l1_serial";
        path = "Program_Labsheet\Program Labsheet 1\Value Flex di Serial Monitor\Value Flex di Serial Monitor.ino"
    },
    @{
        id = "l1_lcd";
        path = "Program_Labsheet\Program Labsheet 1\Value Flex + Sudut + Tegangan di LCD\Value Flex + Sudut + Tegangan di LCD.ino"
    },
    @{
        id = "l2_led";
        path = "Program_Labsheet\Program Labsheet 2\Flex + Led\Flex + Led.ino"
    },
    @{
        id = "l2_servo";
        path = "Program_Labsheet\Program Labsheet 2\Flex + Servo\Flex + Servo.ino"
    },
    @{
        id = "l2_servo_led";
        path = "Program_Labsheet\Program Labsheet 2\Flex + Servo + Lampu\Flex + Servo + Lampu.ino"
    },
    @{
        id = "l2_servo_led_lcd";
        path = "Program_Labsheet\Program Labsheet 2\Flex + Servo + Lampu + LCD\Flex + Servo + Lampu + LCD.ino"
    },
    @{
        id = "l3_serial_servo";
        path = "Program_Labsheet\Program Labsheet 3\Flex + Serial + Servo\Flex + Serial + Servo.ino"
    },
    @{
        id = "l3_wifi_servo";
        path = "Program_Labsheet\Program Labsheet 3\Flex + Wifi + Servo\Flex + Wifi + Servo.ino"
    },
    @{
        id = "l3_dual_servo";
        path = "Program_Labsheet\Program Labsheet 3\Flex + Serial + Wifi + Servo\Flex + Serial + Wifi + Servo.ino"
    }
)

try {
    foreach ($s in $sketches) {
        $sketchPath = Join-Path $projectRoot $s.path
        if (!(Test-Path $sketchPath)) {
            Write-Warning "Sketch not found: $sketchPath"
            continue
        }

        Write-Output "--------------------------------------------------------"
        Write-Output "Compiling $($s.id) : $($s.path)"
        Write-Output "--------------------------------------------------------"

        # Read sketch content
        $content = Get-Content $sketchPath -Raw
        
        # Prepend Arduino.h if not present (since PlatformIO requires it)
        if ($content -notmatch "#include <Arduino.h>") {
            $content = "#include <Arduino.h>`n" + $content
        }

        # Write to src/main.cpp
        Set-Content -Path $srcFile -Value $content -Force

        # Run PlatformIO compile
        Push-Location $firmwareDir
        pio run --environment esp32dev
        Pop-Location

        # Copy compiled binary
        $compiledBin = Join-Path $firmwareDir ".pio\build\esp32dev\firmware.bin"
        if (Test-Path $compiledBin) {
            $targetBin = Join-Path $binariesDir "$($s.id).bin"
            Copy-Item $compiledBin $targetBin -Force
            Write-Output "SUCCESS: Saved to $targetBin"
        } else {
            Write-Error "ERROR: Compiled binary not found for $($s.id)"
        }
    }
} finally {
    # Restore main.cpp
    if (Test-Path $backupFile) {
        Copy-Item $backupFile $srcFile -Force
        Remove-Item $backupFile -Force
        Write-Output "Restored main.cpp"
    }
}
