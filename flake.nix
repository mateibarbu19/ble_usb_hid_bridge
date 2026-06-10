{
  description = "Simple C/C++ dev environment";

  inputs = {
    nixpkgs.url = "nixpkgs/26.05";
  };

  outputs = {self, nixpkgs}:
  let
    pkgs = nixpkgs.legacyPackages.x86_64-linux;

    pico-sdk-full = pkgs.pico-sdk.override { 
      withSubmodules = true; 
    };
  in
  {
    devShells.x86_64-linux.default = pkgs.mkShell {
      buildInputs = with pkgs; [
        llvmPackages_latest.clang-tools # Order
        # llvmPackages_latest.clang       # matters

        gcc-arm-embedded
        python3

        cmake
        gnumake

        pico-sdk-full
        picotool

        gdb
      ];

      env.PICO_SDK_PATH = "${pico-sdk-full}/lib/pico-sdk";
    };
  };
}
