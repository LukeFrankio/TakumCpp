param(
    [string]$BuildDir = "build",
    [string]$BuildType = "Debug"
)

Write-Host "[INFO] Build directory: $BuildDir"
Write-Host "[INFO] Build type: $BuildType"

# --- find executables ---
function Find-Exe($name) {
    $path = (Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    return $path
}

$NinjaPath = Find-Exe "ninja.exe"
$GppPath   = Find-Exe "g++.exe"
$GccPath   = Find-Exe "gcc.exe"
$MakePath  = Find-Exe "mingw32-make.exe"

if (-not $GppPath -or -not $GccPath) {
    Write-Error "[ERROR] g++/gcc not found in PATH. Ensure MSYS2 MinGW64 is installed and added to PATH."
    exit 1
}

# --- decide generator ---
$Generator = $null
$CMakeMakeProgramFlag = ""

if ($NinjaPath) {
    if ($NinjaPath -notmatch "msys|mingw|usr") {
        $Generator = "Ninja"
        $CMakeMakeProgramFlag = "-DCMAKE_MAKE_PROGRAM=`"$NinjaPath`""
        Write-Host "[INFO] Using Windows Ninja: $NinjaPath"
    } elseif ($MakePath) {
        $Generator = "MinGW Makefiles"
        $CMakeMakeProgramFlag = "-DCMAKE_MAKE_PROGRAM=`"$MakePath`""
        Write-Host "[INFO] Using MinGW Makefiles with: $MakePath"
    } else {
        $Generator = "Ninja"
        $CMakeMakeProgramFlag = "-DCMAKE_MAKE_PROGRAM=`"$NinjaPath`""
        Write-Warning "[WARN] Using MSYS Ninja ($NinjaPath). Path mangling issues may occur."
        $env:MSYS2_ARG_CONV_EXCL="*"
        $env:CHERE_INVOKING="1"
    }
} elseif ($MakePath) {
    $Generator = "MinGW Makefiles"
    $CMakeMakeProgramFlag = "-DCMAKE_MAKE_PROGRAM=`"$MakePath`""
    Write-Host "[INFO] Using MinGW Makefiles: $MakePath"
} else {
    Write-Error "[ERROR] No Ninja or mingw32-make.exe found. Please install one."
    exit 1
}

Write-Host "[INFO] C compiler: $GccPath"
Write-Host "[INFO] C++ compiler: $GppPath"

# --- create build dir ---
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# --- run cmake configure ---
Write-Host "[INFO] Configuring project..."
$cmakeArgs = @(
    "-S", ".", "-B", $BuildDir,
    "-G", $Generator,
    $CMakeMakeProgramFlag,
    "-DCMAKE_C_COMPILER=$GccPath",
    "-DCMAKE_CXX_COMPILER=$GppPath",
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

$configure = Start-Process cmake -ArgumentList $cmakeArgs -NoNewWindow -Wait -PassThru
if ($configure.ExitCode -ne 0) {
    Write-Error "[ERROR] CMake configuration failed."
    exit 1
}

# --- build ---
Write-Host "[INFO] Building..."
$build = Start-Process cmake -ArgumentList @("--build", $BuildDir, "--config", $BuildType, "--parallel") -NoNewWindow -Wait -PassThru
if ($build.ExitCode -ne 0) {
    Write-Error "[ERROR] Build failed."
    exit 1
}

# --- run tests ---
Write-Host "[INFO] Running tests..."
Push-Location $BuildDir
$env:CTEST_OUTPUT_ON_FAILURE="1"
$test = Start-Process ctest -ArgumentList "--output-on-failure" -NoNewWindow -Wait -PassThru
Pop-Location
if ($test.ExitCode -ne 0) {
    Write-Error "[FAIL] Some tests failed."
    exit 2
}

Write-Host "[SUCCESS] Build + tests completed successfully in '$BuildDir' ($BuildType)."
exit 0
