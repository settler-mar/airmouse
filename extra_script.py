import os
import gzip
import shutil

from pathlib import Path

Import("env")

SOURCE_DIR = Path("data_source")
DEST_DIR = Path("data")
EXCLUDED_EXTENSIONS = {".bin", ".ir", ".data"}

def compress_file(source_path, dest_path):
    with open(source_path, 'rb') as f_in:
        with gzip.open(dest_path, 'wb', compresslevel=9) as f_out:
            shutil.copyfileobj(f_in, f_out)

def copy_file(source_path, dest_path):
    shutil.copy2(source_path, dest_path)

def process_directory(source_dir, dest_dir):
    if not dest_dir.exists():
        dest_dir.mkdir(parents=True)

    for item in source_dir.iterdir():
        if item.name.startswith(".") or item.name == "mock":
            # Пропускаем скрытые файлы/папки и папку "mock"
            continue

        dest_item = dest_dir / item.name

        if item.is_dir():
            process_directory(item, dest_item)
        else:
            ext = item.suffix.lower()
            if ext in EXCLUDED_EXTENSIONS:
                # Просто копируем файл без сжатия
                copy_file(item, dest_item)
            else:
                # Сжимаем файл с добавлением .gz
                dest_gz = dest_item.with_name(dest_item.name + ".gz")
                compress_file(item, dest_gz)

def ensure_data_dir(source, target, env):
    # Просто убедимся, что папка data существует
    if not DEST_DIR.exists():
        print("==> Creating data directory for build")
        DEST_DIR.mkdir(parents=True)

def before_uploadfs(source, target, env):
    print("==> Preparing data directory...")

    # Очищаем предыдущую папку data
    if DEST_DIR.exists():
        shutil.rmtree(DEST_DIR)

    # Запускаем обработку
    process_directory(SOURCE_DIR, DEST_DIR)

    print("==> Data preparation completed.")

# Добавляем хук для uploadfs
env.AddPreAction("uploadfs", before_uploadfs)

# Добавляем хук для сборки
env.AddPreAction("build", ensure_data_dir)
