import os

SD_PATH = "/media/paddy/SD CARD"
ALBUMS_FOLDER = os.path.join(SD_PATH, "Albums")
PLAYLISTS_FOLDER = os.path.join(SD_PATH, "Playlists")

def generate_index_files(base_path):
	if not os.path.exists(base_path):
		print(f"SD card not found at {base_path}")
		return

	for root, dirs, files in os.walk(base_path):
		index_path = os.path.join(root, "index.txt")

		# Skip if index.txt already exists
		if os.path.exists(index_path):
			print(f"Skipping {root} (index.txt already exists)")
			continue

		# Filter only MP3 files in this folder
		mp3_files = [f for f in files if f.lower().endswith(".mp3")]

		if mp3_files:
			mp3_files.sort()
			with open(index_path, "w", encoding="utf-8") as idx:
				for f in mp3_files:
					full_path = os.path.join(root, f)
					# Force path to start from /Albums
					relative_path = full_path.replace("\\", "/")
					if "/Albums" in relative_path:
						relative_path = relative_path[relative_path.index("/Albums"):]
					idx.write(relative_path + "\n")

			print(f"Created {index_path} with {len(mp3_files)} songs.")



def generate_master_index(sd_root, albums_folder):
	if not os.path.exists(albums_folder):
		print(f"Albums folder not found at {albums_folder}")
		return

	all_songs = []

	for root, dirs, files in os.walk(albums_folder):
		for f in files:
			if f.lower().endswith(".mp3"):
				full_path = os.path.join(root, f)
				relative_path = os.path.relpath(full_path, sd_root)
				all_songs.append(relative_path)

	all_songs.sort()

	master_index_path = os.path.join(sd_root, "index.txt")
	with open(master_index_path, "w", encoding="utf-8") as idx:
		for song in all_songs:
			idx.write("/"+ song + "\n")

	print(f"\nCreated master index: {master_index_path} ({len(all_songs)} songs total)")

def generate_albums_list(sd_root, albums_folder):
	if not os.path.exists(albums_folder):
		print(f"Albums folder not found at {albums_folder}")
		return

	album_paths = []

	for entry in os.scandir(albums_folder):
		if entry.is_dir():
			relative_path = os.path.relpath(entry.path, sd_root)
			album_paths.append(relative_path)

	# Sort alphabetically, ignoring case
	album_paths.sort(key=str.lower)

	albums_txt_path = os.path.join(sd_root, "albums.txt")
	with open(albums_txt_path, "w", encoding="utf-8") as f:
		for album in album_paths:
			f.write("/" + album + "\n")

	print(f"Created albums list: {albums_txt_path} ({len(album_paths)} albums)")


def generate_playlists_list(sd_root, playlists_folder):
	if not os.path.exists(playlists_folder):
		print(f"Playlists folder not found at {playlists_folder}")
		return

	playlist_paths = []

	for entry in os.scandir(playlists_folder):
		if entry.is_dir():
			relative_path = os.path.relpath(entry.path, sd_root)
			playlist_paths.append(relative_path)

	playlist_paths.sort()

	playlists_txt_path = os.path.join(sd_root, "playlists.txt")
	with open(playlists_txt_path, "w", encoding="utf-8") as f:
		for playlist in playlist_paths:
			f.write("/"+playlist + "\n")

	print(f"Created playlists list: {playlists_txt_path} ({len(playlist_paths)} playlists)")

if __name__ == "__main__":
	generate_index_files(ALBUMS_FOLDER)
	generate_master_index(SD_PATH, ALBUMS_FOLDER)
	generate_albums_list(SD_PATH, ALBUMS_FOLDER)
	generate_playlists_list(SD_PATH, PLAYLISTS_FOLDER)
