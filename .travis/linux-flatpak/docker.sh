#!/bin/bash -ex

CITRA_SRC_DIR="/citra"
BUILD_DIR="$CITRA_SRC_DIR/build"
REPO_DIR="$CITRA_SRC_DIR/repo"
STATE_DIR="$CITRA_SRC_DIR/.flatpak-builder"
KEYS_ARCHIVE="/tmp/keys.tar"
SSH_KEY="/tmp/ssh.key"
GPG_KEY="/tmp/gpg.key"

# Update the host packages
apt-get install -y flatpak flatpak-builder ca-certificates git sshfs curl fuse dnsutils gnupg2
flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install -y flathub org.kde.Platform//5.11 org.kde.Sdk//5.11 org.freedesktop.Sdk.Extension.gcc7

# Extract keys
openssl aes-256-cbc -K $encrypted_b4d9fde682ca_key -iv $encrypted_b4d9fde682ca_iv -in "$CITRA_SRC_DIR/.travis/linux-flatpak/keys.tar.enc" -out "$KEYS_ARCHIVE" -d
tar -C /tmp -xvf $KEYS_ARCHIVE

# Configure SSH keys
eval "$(ssh-agent -s)"
chmod -R 600 "$HOME/.ssh"
chown -R root "$HOME/.ssh"
chmod 600 "$SSH_KEY"
ssh-add "$SSH_KEY"
echo "[$SSH_HOSTNAME]:$SSH_PORT,[$(dig +short $SSH_HOSTNAME)]:$SSH_PORT $SSH_PUBLIC_KEY" > ~/.ssh/known_hosts

# Configure GPG keys
gpg2 --import "$GPG_KEY"

# Download the Citra compatibility list
curl --url https://api.citra-emu.org/gamedb/ -o "$CITRA_SRC_DIR/.travis/linux-flatpak/compatibility_list.json"

# Mount our flatpak repository
mkdir -p "$REPO_DIR"
sshfs "$SSH_USER@$SSH_HOSTNAME:$SSH_LOCATION" "$REPO_DIR" -C -p "$SSH_PORT" -o IdentityFile="$SSH_KEY"

# Build the citra flatpak
flatpak-builder -v --jobs=4 --ccache --force-clean --state-dir="$STATE_DIR" --gpg-sign="$GPG_PUBLIC_KEY" --repo="$REPO_DIR" "$BUILD_DIR" "$CITRA_SRC_DIR"/.travis/linux-flatpak/org.citra.citra-nightly.json
flatpak build-update-repo "$REPO_DIR" -v --generate-static-deltas --gpg-sign="$GPG_PUBLIC_KEY"
