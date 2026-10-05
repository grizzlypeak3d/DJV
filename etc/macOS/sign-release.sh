#!/bin/sh

# Sign and notarize the macOS packages continuous integration built.
#
# Usage: sh etc/macOS/sign-release.sh [options] <package.dmg>...
#
#   --output <directory>   Where the signed packages go (default: signed)
#   --upload <tag>         Add them to that GitHub release, with gh
#   --no-notarize          Sign and package only
#   --identity <identity>  Sign with this instead of DJV_MACOS_TEAM_ID; "-"
#                          signs ad hoc, for trying the script
#
# The packages are built unsigned, by the macos-package job, so that the
# signing identity and the notarization credentials never leave this
# machine: they are the only part of a release still made here. A disk image
# cannot be signed after the fact -- the application inside has to be signed
# first, and the image made from that -- so the application is taken out of
# each package, signed, and packaged again under the same name.
#
# Signing uses the Developer ID in DJV_MACOS_TEAM_ID. Notarization uses the
# keychain profile in DJV_MACOS_NOTARY_PROFILE when that is set, made once
# with "xcrun notarytool store-credentials", and otherwise the Apple ID in
# DJV_MACOS_USER_ID and the app specific password in DJV_MACOS_USER_PASSWORD.

set -e

OUTPUT=signed
UPLOAD=
NOTARIZE=1
IDENTITY=$DJV_MACOS_TEAM_ID
IDENTIFIER=com.grizzlypeak3d.djv

while [ $# -gt 0 ]; do
    case "$1" in
        --output) OUTPUT=$2; shift 2 ;;
        --upload) UPLOAD=$2; shift 2 ;;
        --no-notarize) NOTARIZE=; shift ;;
        --identity) IDENTITY=$2; shift 2 ;;
        --*) echo "Unknown option: $1" >&2; exit 1 ;;
        *) break ;;
    esac
done
if [ $# -eq 0 ]; then
    echo "Usage: sh etc/macOS/sign-release.sh [options] <package.dmg>..." >&2
    exit 1
fi
if [ -z "$IDENTITY" ]; then
    echo "DJV_MACOS_TEAM_ID is not set, and no --identity was given" >&2
    exit 1
fi
if [ -n "$NOTARIZE" ] && [ -z "$DJV_MACOS_NOTARY_PROFILE" ] && \
    { [ -z "$DJV_MACOS_USER_ID" ] || [ -z "$DJV_MACOS_USER_PASSWORD" ]; }; then
    echo "Notarization needs DJV_MACOS_NOTARY_PROFILE, or DJV_MACOS_USER_ID" \
        "and DJV_MACOS_USER_PASSWORD" >&2
    exit 1
fi

mkdir -p "$OUTPUT"
WORK=$(mktemp -d)
MOUNT=
cleanup() {
    if [ -n "$MOUNT" ]; then
        hdiutil detach "$MOUNT" -quiet || true
    fi
    rm -rf "$WORK"
}
trap cleanup EXIT

for PACKAGE in "$@"; do
    NAME=$(basename "$PACKAGE")
    echo "== $NAME"
    case "$NAME" in
        *-dev*|*-dirty*)
            echo "$NAME is a development build, not a release" >&2
            exit 1 ;;
    esac

    # The application, and the name of the volume it was on, out of the
    # package as built.
    MOUNT=$WORK/mount
    mkdir -p "$MOUNT"
    hdiutil attach "$PACKAGE" -nobrowse -readonly -mountpoint "$MOUNT" -quiet
    VOLUME=$(diskutil info "$MOUNT" | sed -n 's/^ *Volume Name: *//p')
    STAGE=$WORK/stage
    rm -rf "$STAGE"
    mkdir -p "$STAGE"
    COUNT=0
    for APP in "$MOUNT"/*.app; do
        [ -d "$APP" ] || continue
        ditto "$APP" "$STAGE/$(basename "$APP")"
        COUNT=$((COUNT + 1))
    done
    hdiutil detach "$MOUNT" -quiet
    MOUNT=
    if [ $COUNT -eq 0 ]; then
        echo "No application in $NAME" >&2
        exit 1
    fi

    # Signed as cmake/Modules/macOSAppSign.cmake signs it in a build made
    # here.
    for APP in "$STAGE"/*.app; do
        codesign --sign "$IDENTITY" --timestamp --force --options runtime \
            --identifier $IDENTIFIER --deep "$APP"
        codesign --verify --deep --strict "$APP"
    done

    # The same layout the build's package has: the application beside a
    # link to the Applications folder.
    ln -s /Applications "$STAGE/Applications"
    SIGNED=$OUTPUT/$NAME
    rm -f "$SIGNED"
    hdiutil create -volname "$VOLUME" -srcfolder "$STAGE" -format UDZO \
        -imagekey zlib-level=9 \
        -ov -quiet "$SIGNED"
    codesign --sign "$IDENTITY" --timestamp --force --options runtime \
        --identifier $IDENTIFIER "$SIGNED"

    if [ -n "$NOTARIZE" ]; then
        if [ -n "$DJV_MACOS_NOTARY_PROFILE" ]; then
            xcrun notarytool submit "$SIGNED" \
                --keychain-profile "$DJV_MACOS_NOTARY_PROFILE" --wait
        else
            xcrun notarytool submit "$SIGNED" \
                --apple-id "$DJV_MACOS_USER_ID" \
                --team-id "$DJV_MACOS_TEAM_ID" \
                --password "$DJV_MACOS_USER_PASSWORD" --wait
        fi
        # The same file submitted: stapling another leaves this one
        # unstapled while every command reports success.
        xcrun stapler staple "$SIGNED"
        xcrun spctl --assess --type open \
            --context context:primary-signature --ignore-cache --verbose=2 \
            "$SIGNED"
    fi
    echo "$SIGNED"
done

if [ -n "$UPLOAD" ]; then
    for PACKAGE in "$@"; do
        gh release upload "$UPLOAD" "$OUTPUT/$(basename "$PACKAGE")" --clobber
    done
fi
