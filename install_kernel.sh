#!/usr/bin/env bash

KERNEL=kernel8

echo "Rebuild kernel..."
sudo make -j6 Image.gz modules dtbs

echo "Install modules..."
sudo make -j6 modules_install

echo "Installing custom kernel image as $KERNEL.img..."
sudo cp arch/arm64/boot/Image.gz /boot/firmware/"$KERNEL".img

echo "Copying DTBs..."
sudo cp arch/arm64/boot/dts/broadcom/*.dtb /boot/firmware/

echo "Copying device tree overlays..."
sudo cp arch/arm64/boot/dts/overlays/*.dtb* /boot/firmware/overlays/
sudo cp arch/arm64/boot/dts/overlays/README /boot/firmware/overlays/

echo "Kernel installation complete!"
