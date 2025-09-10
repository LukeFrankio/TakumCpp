param(
    [string]$BuildDir = "build",
    [string]$BuildType = "Debug"
)

# If possible, re-invoke this script under ExecutionPolicy Bypass so users
# can run the build without changing their system policy permanently.
# Note: this only works when the script is allowed to start; a policy that
# prevents script execution entirely will still block the first invocation.
try {
    $procPolicy = Get-ExecutionPolicy -Scope Process -ErrorAction SilentlyContinue
} catch {
    $procPolicy = $null
}
if ($procPolicy -ne 'Bypass') {
    $pwsh = (Get-Command powershell.exe -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if ($pwsh) {
        # Construct argument list: forward original args
        $forward = @()
        if ($BuildDir) { $forward += "-BuildDir"; $forward += $BuildDir }
        if ($BuildType) { $forward += "-BuildType"; $forward += $BuildType }
        Write-Host "[INFO] Re-invoking script with ExecutionPolicy Bypass"
        $startInfo = Start-Process -FilePath $pwsh -ArgumentList @("-NoProfile","-ExecutionPolicy","Bypass","-File",(Resolve-Path $MyInvocation.MyCommand.Path)) + $forward -NoNewWindow -Wait -PassThru
        exit $startInfo.ExitCode
    }
}

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

# --- generate phi_coeffs.h (optional pre-step to avoid first build race) ---
$phiGenScript = Join-Path (Get-Location) "scripts/gen_poly_coeffs.py"
$phiHeader = Join-Path (Get-Location) "include/takum/internal/generated/phi_coeffs.h"
if (Test-Path $phiGenScript) {
    $py = (Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if (-not $py) { $py = (Get-Command python3 -ErrorAction SilentlyContinue | Select-Object -First 1).Source }
    if ($py) {
        Write-Host "[INFO] Generating Φ coefficients via $py $phiGenScript"
        & $py $phiGenScript $phiHeader
    } else {
        Write-Host "[WARN] Python interpreter not found; skipping coefficient regeneration"
    }
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

# --- optional: run Doxygen if available ---
function Find-ExePath($name, $hints) {
    $p = (Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if ($p) { return $p }
    foreach ($h in $hints) {
        $candidate = Join-Path $h "$name"
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

$doxygenPath = Find-ExePath "doxygen.exe" @("C:\Program Files\doxygen\bin","C:\Program Files (x86)\doxygen\bin","C:\ProgramData\chocolatey\bin")
if ($doxygenPath) {
    Write-Host "[INFO] Running Doxygen: $doxygenPath"
    $dox = Start-Process -FilePath $doxygenPath -ArgumentList "Doxyfile" -NoNewWindow -Wait -PassThru
    if ($dox.ExitCode -ne 0) {
        Write-Warning "[WARN] Doxygen exited with code $($dox.ExitCode). See output above."
    } else {
        Write-Host "[INFO] Documentation generated in 'docs/doxygen'."
    }
} else {
    Write-Host "[INFO] Doxygen not found; skipping documentation generation."
}

exit 0
