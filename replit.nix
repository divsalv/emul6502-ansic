{ pkgs }: {
	deps = [
   pkgs.ascii
   pkgs.nano
		pkgs.clang_12
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}