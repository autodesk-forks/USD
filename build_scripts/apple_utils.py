#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Utilities for managing Apple OS build concerns.
#
# NOTE: This file and its contents may change significantly as we continue
# working to make the build scripts more modular. We anticipate providing
# a clearer and more extensible way of expressing platform specific concerns
# as we add support for additional platforms.

import sys
import locale
import os
import re
import platform
import shlex
import subprocess
import shutil
from typing import Optional, List, Dict


TARGET_NATIVE = "native"
TARGET_X86 = "x86_64"
TARGET_ARM64 = "arm64"
TARGET_UNIVERSAL = "universal"
TARGET_IOS = "iOS"
TARGET_IOS_SIMULATOR = "iOSSimulator"
TARGET_VISIONOS = "visionOS"
TARGET_VISIONOS_SIMULATOR = "visionOSSimulator"

EMBEDDED_PLATFORMS = [TARGET_IOS, TARGET_IOS_SIMULATOR,
                      TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR]

def GetBuildTargets():
    return [
        TARGET_NATIVE,
        TARGET_X86,
        TARGET_ARM64,
        TARGET_UNIVERSAL,
        TARGET_IOS,
        TARGET_IOS_SIMULATOR,
        TARGET_VISIONOS,
        TARGET_VISIONOS_SIMULATOR
    ]

def normalizeBuildTarget(target: str) -> Optional[str]:
    """Returns a case-normalized build target name, or
    None if the target is not recognized."""
    targetLower = target.casefold()
    for validTarget in GetBuildTargets():
        if targetLower == validTarget.casefold():
            return validTarget
    return None

def GetBuildTargetDefault():
    return TARGET_NATIVE

def MacOS():
    return platform.system() == "Darwin"

def TargetEmbeddedOS(context):
    targetLower = context.buildTarget.casefold()
    return targetLower in map(str.casefold, EMBEDDED_PLATFORMS)

def GetLocale():
    return sys.stdout.encoding or locale.getdefaultlocale()[1] or "UTF-8"

def GetCommandOutput(command, **kwargs):
    """Executes the specified command and returns output or None."""
    try:
        return subprocess.check_output(
            command, stderr=subprocess.STDOUT, **kwargs).decode(
                                        GetLocale(), 'replace').strip()
    except:
        return None

def GetTargetArmArch():
    # Allows the arm architecture string to be overridden by
    # setting MACOS_ARM_ARCHITECTURE
    return os.environ.get('MACOS_ARM_ARCHITECTURE') or TARGET_ARM64

def GetHostArch():
    macArch = GetCommandOutput(["arch"])
    if macArch == "i386" or macArch == TARGET_X86:
        macArch = TARGET_X86
    else:
        macArch = GetTargetArmArch()
    return macArch

def GetTargetArch(context):
    if TargetEmbeddedOS(context):
        return GetTargetArmArch()

    if context.targetNative:
        return GetHostArch()
    
    if context.targetX86:
        return TARGET_X86
    if context.targetARM64:
        return GetTargetArmArch()
    if context.targetUniversal:
        return TARGET_X86 + ";" + GetTargetArmArch()

    return None

def IsHostArm():
    return GetHostArch() != TARGET_X86

def IsTargetArm(context):
    return GetTargetArch(context) != TARGET_X86

def GetTargetArchPair(context):
    secondaryArch = None

    if context.targetNative:
        primaryArch = GetHostArch()
    if context.targetX86:
        primaryArch = TARGET_X86
    if context.targetARM64:
        primaryArch = GetTargetArmArch()
    if TargetEmbeddedOS(context):
        primaryArch = GetTargetArmArch()
    if context.targetUniversal:
        primaryArch = GetHostArch()
        if (primaryArch == TARGET_X86):
            secondaryArch = GetTargetArmArch()
        else:
            secondaryArch = TARGET_X86

    return (primaryArch, secondaryArch)

def SupportsMacOSUniversalBinaries():
    if not MacOS():
        return False
    XcodeVersion = GetXcodeVersion()[0]
    return XcodeVersion > (11, 0)

def GetSDKName(context) -> str:
    sdk = "macosx"
    if context.buildTarget == TARGET_IOS:
        sdk = "iPhoneOS"
    elif context.buildTarget == TARGET_IOS_SIMULATOR:
        sdk = "iPhoneSimulator"
    elif context.buildTarget == TARGET_VISIONOS:
        sdk = "xrOS"
    elif context.buildTarget == TARGET_VISIONOS_SIMULATOR:
        sdk = "xrSimulator"

    return sdk

def GetSDKRoot(context) -> Optional[str]:
    sdk = GetSDKName(context).lower()

    for arg in (context.cmakeBuildArgs or '').split():
        if "CMAKE_OSX_SYSROOT" in arg:
            override = arg.split('=')[1].strip('"').strip()
            if override:
                sdk = override

    sdkroot = GetCommandOutput(["xcrun", "--sdk", sdk, "--show-sdk-path"])
    if not sdkroot:
        raise RuntimeError(f"Could not find an sdk path. Make sure you have the {sdk} sdk installed.")
    return sdkroot

def GetSDKVersion(context):
    sdk_basename = os.path.basename(GetSDKRoot(context))
    return re.search(r'\d+\.\d+', sdk_basename).group()

def SetTarget(context):
    targetName = normalizeBuildTarget(context.buildTarget)

    # In the case that normalizeBuildTarget returns no value, we are cross 
    # compiling and none of the targets set below are relevant. We want to
    # explicitly preserve the buildTarget for further consumers of context.
    if targetName is None:
        return

    context.buildTarget = targetName
    context.targetNative = (targetName == TARGET_NATIVE)
    context.targetX86 = (targetName == TARGET_X86)
    context.targetARM64 = (targetName == GetTargetArmArch())
    context.targetUniversal = (targetName == TARGET_UNIVERSAL)
    context.targetIOS = (targetName in (TARGET_IOS, TARGET_IOS_SIMULATOR))
    context.targetVisionOS = (
        targetName in (TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR))
    context.targetSimulator = (
        targetName in (TARGET_IOS_SIMULATOR, TARGET_VISIONOS_SIMULATOR))
    if context.targetUniversal and not SupportsMacOSUniversalBinaries():
        context.targetUniversal = False
        raise ValueError(
            "Universal binaries only supported in macOS 11.0 and later.")

def GetTargetName(context):
    return (TARGET_NATIVE if context.targetNative else
            TARGET_X86 if context.targetX86 else
            GetTargetArmArch() if context.targetARM64 else
            TARGET_UNIVERSAL if context.targetUniversal else
            context.buildTarget)

def GetTargetPlatform(context):
    return GetTargetName(context).replace("Simulator", "")

devout = open(os.devnull, 'w')

def ExtractFilesRecursive(path, cond):
    files = []
    for r, d, f in os.walk(path):
        for file in f:
            if cond(os.path.join(r,file)):
                files.append(os.path.join(r, file))
    return files

def _GetCodeSignStringFromTerminal():
    """Return the output from the string codesigning variables"""
    codeSignIDs = GetCommandOutput(
        ['security', 'find-identity', '-vp', 'codesigning'])
    return codeSignIDs


def GetXcodeVersion():
    output = GetCommandOutput(['xcodebuild', '-version']).split()
    version = tuple(int(f) for f in output[1].split("."))
    build = output[-1]

    return version, build


def GetCodeSigningIdentifiers() -> Dict[str, str]:
    """Returns a dictionary of codesigning identifiers and their hashes"""
    XcodeVersion = GetXcodeVersion()[0]
    codeSignIDs = _GetCodeSignStringFromTerminal()

    identifiers = {}
    for codeSignID in (codeSignIDs or "").splitlines():
        if "CSSMERR_TP_CERT_REVOKED" in codeSignID:
            continue
        if ")" not in codeSignID:
            continue
        if ((XcodeVersion >= (11, 0) and "Apple Development" in codeSignID)
            or "Mac Developer" in codeSignID):
            identifier = codeSignID.split()[1]
            identifier_hash = re.search(r'\(.*?\)', codeSignID)
            if identifier_hash:
                identifier_hash = identifier_hash[0][1:-1]
            else:
                identifier_hash = None

            identifiers[identifier] = identifier_hash

    identifiers["-"] = None
    return identifiers


def GetCodeSignID() -> str:
    """Return the first code signing identifier"""
    identifiers = GetCodeSigningIdentifiers()
    env_signing_id = os.environ.get('CODE_SIGN_ID')
    if env_signing_id:
        if env_signing_id in identifiers:
            return env_signing_id
        raise RuntimeError(
            f"Could not find environment specified identifier "
            f"{env_signing_id} in registered code signing identifiers")

    return list(GetCodeSigningIdentifiers().keys())[0]


def GetDevelopmentTeamID(identifier=None):
    if "DEVELOPMENT_TEAM" in os.environ:
        return os.environ.get("DEVELOPMENT_TEAM")

    if not identifier:
        identifier = GetCodeSignID()
    if identifier == "-":
        return None

    identifier_hash = GetCodeSigningIdentifiers().get(identifier)
    if not identifier_hash:
        raise RuntimeError("Could not get identifiers hash")

    certs = subprocess.check_output(
        ["security", "find-certificate", "-c", identifier_hash, "-p"])
    subject = GetCommandOutput(["openssl", "x509", "-subject"], input=certs)
    subject = subject.splitlines()[0]
    match = re.search(r"OU\s*=\s*(?P<team>([A-Za-z0-9_])+)", subject)
    if not match:
        raise RuntimeError("Could not parse the output "
                           "certificate to find the team ID")

    groups = match.groupdict()
    team = groups.get("team")

    if not team:
        raise RuntimeError("Could not extract team id from certificate")

    return team


def CodesignPath(path, identifier, team_identifier,
                 force=False, is_framework=False) -> bool:
    resign = force
    if not force:
        codesigning_info = GetCommandOutput(["codesign", "-vd", path])
        if not codesigning_info:
            resign = True
        else:
            # The output has multiple lines here
            for line in codesigning_info.splitlines():
                if line.startswith("TeamIdentifier="):
                    current_team_identifier = line.split("=")[-1]
                    if (not current_team_identifier 
                        or "not set" in current_team_identifier):
                        resign = True
                        break
                    elif current_team_identifier == team_identifier:
                        break
            else:
                resign = True

    if not resign:
        return False

    # Frameworks need to be signed with different parameters than loose binaries
    if is_framework:
        subprocess.check_call(
            ["codesign", "--force", "--sign", identifier,
             "--generate-entitlement-der", "--verbose", path])
    else:
        subprocess.check_call(
            ["codesign", "--force", "--sign", identifier, path],
            stdout=devout, stderr=devout)
    return True


def Codesign(install_path, identifier=None, force=False,
             verbose_output=False) -> bool:
    if not MacOS():
        return False

    identifier = identifier or GetCodeSignID()

    if verbose_output:
        global devout
        devout = sys.stdout
        print(f"Code-signing files in {install_path} "
              f"with {identifier}", file=devout)

    try:
        team_identifier = GetDevelopmentTeamID(identifier)
    except:
        if verbose_output:
            print("Could not get team_identifier")
        team_identifier = None

    codesignPaths = [
        os.path.join(install_path, 'lib'),
        os.path.join(install_path, 'plugin'),
        os.path.join(install_path, 'share/usd'),
        os.path.join(install_path, "frameworks")
    ]
        
    for basePath in codesignPaths:
        if not os.path.exists(basePath):
            continue

        for root, dirs, files in os.walk(basePath, topdown=True):
            for f in files:

                _, ext = os.path.splitext(f)
                if ext in (".dylib", ".so"):
                    path = os.path.join(root, f)
                    result = CodesignPath(path, identifier, 
                                          team_identifier=team_identifier, 
                                          force=force, is_framework=False)
                    if verbose_output:
                        if result:
                            print(f"Code-signed binary: {path}")
                        else:
                            print(f"Did not code-sign binary: {path}")

        # Bit annoying to have to do this twice, but seems the fastest way
        # to skip traversing frameworks
        frameworks = [d for d in dirs if d.endswith(".framework")]
        dirs[:] = [d for d in dirs if not d.endswith(".framework")]

        for framework in frameworks:
            framework_name = os.path.splitext(framework)[0]
            if (framework_name.lower() not in 
                ["openusd", "opensubdiv", "materialx"]):
                continue
            path = os.path.join(root, framework)
            result = CodesignPath(path, identifier, 
                                  team_identifier=team_identifier,
                                  force=force, is_framework=True)
            if verbose_output:
                if result:
                    print(f"Code-signed framework: {path}")
                else:
                    print(f"Did not code-sign framework: {path}")

    return True

def CreateUniversalBinaries(context, libNames, x86Dir, armDir):
    if not MacOS():
        return False
    lipoCommands = []
    xcodeRoot = subprocess.check_output(
        ["xcode-select", "--print-path"]).decode('utf-8').strip()
    lipoBinary = \
        "{XCODE_ROOT}/Toolchains/XcodeDefault.xctoolchain/usr/bin/lipo".format(
                XCODE_ROOT=xcodeRoot)
    for libName in libNames:
        outputDir = os.path.join(context.instDir, "lib")
        if not os.path.isdir(outputDir):
            os.mkdir(outputDir)

        outputName = os.path.join(outputDir, libName)
        if not os.path.islink("{x86Dir}/{libName}".format(
                                x86Dir=x86Dir, libName=libName)):
            if os.path.exists(outputName):
                os.remove(outputName)
            lipoCmd = "{lipo} -create {x86Dir}/{libName} {armDir}/{libName} " \
                      "-output {outputName}".format(
                                lipo=lipoBinary,
                                x86Dir=x86Dir, armDir=armDir,
                                libName=libName, outputName=outputName)
            lipoCommands.append(lipoCmd)
            p = subprocess.Popen(shlex.split(lipoCmd))
            p.wait()
    for libName in libNames:
        if os.path.islink("{x86Dir}/{libName}".format(
                                x86Dir=x86Dir, libName=libName)):
            outputName = os.path.join(context.instDir, "lib", libName)
            if os.path.exists(outputName):
                os.unlink(outputName)
            targetName = os.readlink("{x86Dir}/{libName}".format(
                                x86Dir=x86Dir, libName=libName))
            targetName = os.path.basename(targetName)
            os.symlink("{instDir}/lib/{libName}".format(
                                instDir=context.instDir, libName=targetName),
                       outputName)
    return lipoCommands

def ConfigureCMakeExtraArgs(context, args:List[str]) -> List[str]:
    system_name = None
    if TargetEmbeddedOS(context):
        system_name = GetTargetPlatform(context)

    if system_name:
        args.append(f"-DCMAKE_SYSTEM_NAME={system_name}")
        args.append(f"-DCMAKE_OSX_SYSROOT={GetSDKRoot(context)}")

        # Required to find locally built libs not from the sysroot.
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH")
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH")
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH")

    return args

def GetTBBPatches(context):
    if context.buildTarget not in EMBEDDED_PLATFORMS or context.buildTarget == TARGET_IOS:
        # TBB already handles these so we don't patch them out
        return [], []

    sdk_name = GetSDKName(context)

    # Standard Target based names
    target_config_patches = [("ios", context.buildTarget.lower()),
                             ("iOS", context.buildTarget),
                             ("IPHONEOS", sdk_name.upper())]

    clang_config_patches = [("ios",context.buildTarget.lower()),
                            ("iOS", context.buildTarget),
                            ("IPHONEOS",sdk_name.upper())]

    if context.buildTarget in (TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR):
        target_config_patches.extend([("iPhone", "XR"),
                                      ("?= 8.0", "?= 1.0")])

        clang_config_patches.append(("iPhone", "XR"),)

    if context.buildTarget == TARGET_VISIONOS:
        clang_config_patches.append(
            ("-miphoneos-version-min=",
             "-target arm64-apple-xros"))
    else:
        version = GetSDKVersion(context)

        if context.buildTarget == TARGET_VISIONOS_SIMULATOR:
            clang_config_patches.append(
                ("-miphoneos-version-min=",
                 f"-target arm64-apple-xros{version}-simulator"))
        elif context.buildTarget == TARGET_IOS_SIMULATOR:
            clang_config_patches.append(
                ("-miphoneos-version-min=",
                 f"-target arm64-apple-ios{version}-simulator"))

    return target_config_patches, clang_config_patches


def BuildXCFramework(root, targets, args):
    if TARGET_UNIVERSAL in targets:
        targets.extend([TARGET_ARM64, TARGET_X86])
        targets.remove(TARGET_UNIVERSAL)
    if TARGET_NATIVE in targets:
        targets.remove(TARGET_NATIVE)
        targets.append(GetHostArch())

    targets = set(targets)
    print(f"Building {len(targets)} targets...")
    shared_sources = os.path.join(root, "shared_sources")
    os.makedirs(shared_sources, exist_ok=True)

    do_lipo = TARGET_ARM64 in targets and TARGET_X86 in targets

    build_command = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build_usd.py")
    frameworks = []
    to_lipo = []
    for target in targets:
        print(f"Building {target}...")
        install_dir = os.path.join(root, "builds", target)
        target_src_dir = os.path.join(install_dir, "src")
        os.makedirs(target_src_dir, exist_ok=True)
        framework = os.path.join(install_dir, "frameworks/OpenUSD.framework")
        if do_lipo and target in (TARGET_X86, TARGET_ARM64):
            to_lipo.append(framework)
        else:
            frameworks.append(framework)

        # Copy the shared sources over to save time
        for src in os.listdir(shared_sources):
            shared_src = os.path.join(shared_sources, src)
            target_src = os.path.join(target_src_dir, src)
            shutil.copy2(shared_src, target_src)

        target_args = [sys.executable, build_command, install_dir, "--build-target", target, "--build-apple-framework"]
        target_args.extend(args)
        try:
            subprocess.check_call(target_args)
        except:
            raise RuntimeError(f"Failed to build {target} using {' '.join(target_args)}")

        # Copy the unshared sources back as needed
        # We copy the zips in case there are any patches involved
        for src in os.listdir(target_src_dir):
            target_src_path = os.path.join(target_src_dir, src)
            shared_src_path = os.path.join(shared_sources, src)
            if not os.path.exists(shared_src_path) and os.path.isfile(target_src_path):
                shutil.copy2(target_src_path, shared_src_path)

        assert os.path.exists(framework)

    if do_lipo:
        print("Combining Mac framework architectures")
        assert (len(to_lipo) == 2)

        fat_dir = os.path.join(root, "builds/fat")
        if os.path.exists(fat_dir):
            shutil.rmtree(fat_dir)

        fat_framework = os.path.join(fat_dir, "OpenUSD.framework")
        subprocess.check_call(["ditto", to_lipo[0], fat_framework])  # Ditto copies more metadata than shutil does

        dylib_a = os.path.join(to_lipo[0], "Versions/A/OpenUSD")
        dylib_b = os.path.join(to_lipo[1], "Versions/A/OpenUSD")
        dylib_dest = os.path.join(fat_framework, "Versions/A/OpenUSD")
        subprocess.check_call(["lipo", dylib_a, dylib_b, "-create", "-output", dylib_dest])
        frameworks.append(fat_framework)

    print("Creating XCFramework")
    xcframework_dir = os.path.join(root, "xcframework")
    if os.path.exists(xcframework_dir):
        shutil.rmtree(xcframework_dir)
    os.makedirs(xcframework_dir, exist_ok=True)
    xcframework_path = os.path.join(xcframework_dir, "OpenUSD.xcframework")
    command = ["xcodebuild", "-create-xcframework", "-output", xcframework_path]
    for framework in frameworks:
        command.extend(["-framework", framework])

    try:
        subprocess.check_call(command)
    except:
        raise RuntimeError(f"Failed to create XCFramework using {' '.join(command)}")

    print("Success! Add the OpenUSD.xcframework to your Xcode Project.")


def main():
    import argparse
    parser = argparse.ArgumentParser(description="A set of command line utilities for building on Apple Platforms")
    subparsers = parser.add_subparsers(dest="command", required=True)

    xcframework = subparsers.add_parser("xcframework",
                                        description="Build multiple framework targets together as a single xcframework")
    xcframework.add_argument("install_dir", type=str,
                             help="Directory where the XCFramework will be installed")
    xcframework.add_argument("--build-targets", nargs="+", help="The list of targets to build.",
                             choices=GetBuildTargets(),
                             default=GetBuildTargets())

    args, unknown = parser.parse_known_args()
    command = args.command
    if command == "xcframework":
        BuildXCFramework(args.install_dir, args.build_targets, unknown)
    else:
        raise RuntimeError(f"Unknown command: {command}")


if __name__ == '__main__':
    main()
