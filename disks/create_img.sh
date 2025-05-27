#!/bin/bash
set -e

# Function to create a floppy image and add files/folders to it
create_floppy_image() {
  local imgname=$1

  echo "Creating image: $imgname"

  # Create a blank 1.44MB floppy disk image
  mkfs.msdos -C "$imgname" 1440

  # Create a root-level test file
  echo "Test message at the root of $imgname" > root_msg.txt

  # Copy the root-level file into the image
  mcopy -i "$imgname" root_msg.txt ::root_msg.txt

  # Create a directory inside the image
  mmd -i "$imgname" ::mydir

  # Create a test file for the directory
  echo "Test message inside 'mydir' in $imgname" > test_msg.txt

  # Copy the file into the directory
  mcopy -i "$imgname" test_msg.txt ::mydir/test_msg.txt

  # Clean up temporary files
  rm root_msg.txt test_msg.txt

  echo "Done with $imgname"
}

# Create three floppy disk images
create_floppy_image flp0.img
create_floppy_image flp1.img
create_floppy_image flp2.img

echo "All floppy images created successfully!"
