#!/bin/bash
set -e

# Fonction pour créer une image, ajouter fichiers et dossier
create_floppy_image() {
  local imgname=$1

  echo "Création de l'image $imgname"

  # Créer une image floppy 1.44MB vide
  mkfs.msdos -C "$imgname" 1440

  # Fichier à la racine
  echo "Message de test à la racine de $imgname" > root_msg.txt

  # Copier fichier racine
  mcopy -i "$imgname" root_msg.txt ::root_msg.txt

  # Créer dossier dans image
  mmd -i "$imgname" ::mydir

  # Fichier dans le dossier
  echo "Message de test dans mon_dossier de $imgname" > msg.txt

  # Copier fichier dans dossier
  mcopy -i "$imgname" msg.txt ::mydir/msg.txt

  # Nettoyer fichiers temporaires
  rm root_msg.txt msg.txt

  echo "Terminé pour $imgname"
}

# Créer les 3 images
create_floppy_image flp0.img
create_floppy_image flp1.img
create_floppy_image flp2.img

echo "Toutes les images ont été créées avec succès."
