param(
    [Parameter(Mandatory=$true)][string]$Vcpkg,
    [Parameter(Mandatory=$true)][string]$Workspace,
    [Parameter(Mandatory=$true)][string]$AndroidSdk,
    [Parameter(Mandatory=$true)][string]$JavaHome,
    [int]$Jobs=8
)
# Run from a Visual Studio x64 developer PowerShell. All build output stays in Workspace.
$ErrorActionPreference='Stop'
function Run([string]$Program,[string[]]$Arguments) {
    & $Program @Arguments
    if($LASTEXITCODE -ne 0){throw "$Program failed with exit code $LASTEXITCODE"}
}
function Full([string]$Path){return [IO.Path]::GetFullPath($Path).Replace('\','/')}
$overlay=Full (Join-Path $PSScriptRoot '..')
$source=Full (Join-Path $overlay '..')
$dependencies=Full $Vcpkg
$work=Full $Workspace
$sdk=Full $AndroidSdk
$jdk=Full $JavaHome
$ndk="$sdk/ndk/28.2.13676358"
$cmake="$sdk/cmake/3.31.5/bin/cmake.exe"
$ninja="$sdk/cmake/3.31.5/bin/ninja.exe"
foreach($required in @($cmake,$ninja,"$ndk/build/cmake/android.toolchain.cmake","$jdk/bin/javac.exe","$dependencies/vcpkg.exe")){
    if(-not(Test-Path -LiteralPath $required)){throw "Missing build prerequisite: $required"}
}
if(-not(Get-Command cl.exe -ErrorAction SilentlyContinue)){throw 'Use a Visual Studio x64 developer PowerShell with the C++ desktop workload.'}
$python=(Get-Command python.exe -ErrorAction Stop).Source
New-Item -ItemType Directory -Force -Path $work | Out-Null
$env:JAVA_HOME=$jdk
$env:ANDROID_HOME=$sdk
$env:ANDROID_NDK_HOME=$ndk
$env:ANDROID_NDK_ROOT=$ndk
$env:VCPKG_ROOT=$dependencies
$env:PATH="$jdk/bin;$sdk/cmake/3.31.5/bin;$env:PATH"
# A short socket path also works when PowerShell is hosted inside an MSIX app.
$socketDirectory=Join-Path $env:LOCALAPPDATA 'ComboShipJava'
New-Item -ItemType Directory -Force -Path $socketDirectory | Out-Null
$env:JAVA_TOOL_OPTIONS="$env:JAVA_TOOL_OPTIONS -Djdk.net.unixdomain.tmpdir=$socketDirectory"

Run "$dependencies/vcpkg.exe" @('install','--triplet','arm64-android','zlib','libpng','libogg','libvorbis','opus','opusfile')
Run "$dependencies/vcpkg.exe" @('install','--triplet','x64-windows-static-md','zlib','bzip2','libzip','libpng','sdl2','sdl2-net','glew','glfw3','nlohmann-json','tinyxml2','spdlog','libogg','libvorbis','opus','opusfile')
Run $cmake @('-S',$source,'-B',"$work/build-android",'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
    "-DCMAKE_MAKE_PROGRAM=$ninja","-DCMAKE_TOOLCHAIN_FILE=$ndk/build/cmake/android.toolchain.cmake",'-DANDROID_ABI=arm64-v8a',
    '-DANDROID_PLATFORM=android-28','-DANDROID_STL=c++_shared',"-DCOMBO_VCPKG_PREFIX=$dependencies/installed/arm64-android",
    '-DENABLE_SCRIPTING=OFF','-DUSE_OPENGLES=ON')
Run $cmake @('--build',"$work/build-android",'--target','ComboShip','--parallel',"$Jobs")
Run $cmake @('-S',$source,'-B',"$work/build-assets",'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_MAKE_PROGRAM=$ninja","-DCMAKE_TOOLCHAIN_FILE=$dependencies/scripts/buildsystems/vcpkg.cmake",
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static-md','-DENABLE_SCRIPTING=OFF')
Run $cmake @('--build',"$work/build-assets",'--target','GenerateSohOtr','Generate2ShipOtr','--parallel',"$Jobs")
Run $python @('-X','utf8',"$overlay/tools/stage_runtime.py",'--upstream',$source,'--native',"$work/build-android",
    '--sdl',"$work/build-android/_deps/sdl2-src",'--ndk',$ndk,'--app',"$overlay/app")
# Package from clean outputs: incremental packaging leaves megabytes of dead space in the APK.
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue "$overlay/app/build/outputs/apk","$overlay/app/build/intermediates/apk"
Run "$overlay/gradlew.bat" @('-p',$overlay,':app:assembleDebug','--no-daemon')
$apk="$overlay/app/build/outputs/apk/debug/app-debug.apk"
Run $python @('-X','utf8',"$overlay/tools/verify_apk.py",$apk)
$output="$work/apk-output"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$versionMatch=[regex]::Match((Get-Content -LiteralPath "$overlay/app/build.gradle" -Raw), "versionName\s+'([^']+)'")
if(-not $versionMatch.Success){throw 'Missing Android versionName'}
$package="$output/ComboShip-Android-$($versionMatch.Groups[1].Value)-arm64.apk"
Copy-Item -LiteralPath $apk -Destination $package
Run "$sdk/build-tools/35.0.0/apksigner.bat" @('verify','--verbose','--print-certs',$package)
$hash=(Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$output/SHA256SUMS.txt","$hash  $([IO.Path]::GetFileName($package))`n")
Write-Host "Verified APK: $package"
