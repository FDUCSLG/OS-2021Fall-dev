#!/usr/bin/env python3

# TODO: generate with filesystem image.

from os import system
from pathlib import Path
from argparse import ArgumentParser

def sh(command):
    print(f'> {command}')
    assert system(command) == 0

sector_size = 512
n_sectors = 256 * 1024
boot_offset = 2048
n_boot_sectors = 128 * 1024
filesystem_offset = boot_offset + n_boot_sectors
n_filesystem_sectors = n_sectors - filesystem_offset

boot_files = [
    'armstub8-rpi4.bin',
    'bootcode.bin',
    'config.txt',
    'COPYING.linux',
    'fixup_cd.dat',
    'fixup.dat',
    'fixup4.dat',
    'fixup4cd.dat',
    'LICENCE.broadcom',
    'start_cd.elf',
    'start.elf',
    'start4.elf',
    'start4cd.elf'
]

def generate_boot_image(target, images):
    sh(f'dd if=/dev/zero of={target} seek={n_boot_sectors - 1} bs={sector_size} count=1')

    # "-F 32" specifies FAT32.
	# "-s 1" specifies one sector per cluster so that we can create a smaller one.
    sh(f'mkfs.vfat -F 32 -s 1 {target}')

	# copy files into boot partition.
    for image in images:
        sh(f'mcopy -i {target} {image} ::{Path(image).name}')

def generate_sd_image(target, boot_image):
    sh(f'dd if=/dev/zero of={target} seek={n_sectors - 1} bs={sector_size} count=1')

    boot_line = f'{boot_offset}, {n_boot_sectors * sector_size // 1024}K, c,'
    filesystem_line = f'{filesystem_offset}, {n_filesystem_sectors * sector_size // 1024}K, L,'
    sh(f'echo -e "{boot_line}\n{filesystem_line}\n" | sfdisk {target}')

    sh(f'dd if={boot_image} of={target} seek={boot_offset} conv=notrunc')

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('kernel')
    parser.add_argument('root')
    args = parser.parse_args()

    sd_image = f'{args.root}/sd.img'
    boot_image = f'{args.root}/boot.img'

    generate_boot_image(boot_image, [args.kernel] + boot_files)
    generate_sd_image(sd_image, boot_image)
