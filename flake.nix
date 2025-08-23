{
  description = "Sigil C Project Development Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true; # Enable if you need any unfree packages
        };
      in
      {
        devShells.default = pkgs.mkShell {
          name = "sigil-c-dev";

          # Primary development tools
          nativeBuildInputs = with pkgs; [
            # Compilers
            clang
            clang-tools # Includes clangd, clang-format, etc.
            gcc # Keep GCC as alternative
            
            # Build system
            cmake
            cmakeCurses # For curses-based CMake GUI (ccmake)
            
            # Static analysis
            cppcheck
            
            # Debugging tools
            gdb
            lldb
            valgrind
            
            # Build utilities
            gnumake
            ninja
            pkg-config
            
            # Version control & misc
            git
            bear # For generating compile_commands.json
          ];

          # Runtime dependencies (if your project needs specific libraries)
          buildInputs = with pkgs; [
            # Add any library dependencies here
            # glibc
            # openssl
            # zlib
            # ncurses
          ];

          # Environment variables for Clang/CMake
          env = {
            # Prefer clang over gcc
            CC = "clang";
            CXX = "clang++";
            
            # Enable colored output
            CLICOLOR = "1";
            COLORTERM = "truecolor";
            
            # CMake generator (optional - uncomment if you prefer Ninja)
            # CMAKE_GENERATOR = "Ninja";
          };

          # Shell hook - runs when entering the environment
          shellHook = ''
            echo "🔧 Entering Sigil C development environment"
            echo "Compiler: $(clang --version | head -n1)"
            echo "CMake: $(cmake --version | head -n1)"
            echo "Tools available: clang, cmake, cppcheck, gdb, make, ninja"
            
            # Create build directory if it doesn't exist
            if [ ! -d "build" ]; then
              echo "📁 Creating build/ directory"
              mkdir -p build
            fi
            
            # Generate compile_commands.json if using bear and Makefiles exist
            if [ -f "Makefile" ] && command -v bear >/dev/null 2>&1; then
              echo "📝 Run 'bear -- make' to generate compile_commands.json for LSP support"
            fi
          '';
        };

        # Optional: Add a package definition for your project
        packages.default = pkgs.stdenv.mkDerivation {
          name = "sigil";
          src = self;
          
          nativeBuildInputs = with pkgs; [ cmake gnumake ];
          
          configurePhase = ''
            mkdir -p build
            cd build
            cmake ..
          '';
          
          buildPhase = ''
            make -j$NIX_BUILD_CORES
          '';
          
          installPhase = ''
            mkdir -p $out/bin
            cp sigil $out/bin/ # Adjust based on your executable name
          '';
        };

        # Optional: Development app to run your project
        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/sigil";
        };
      }
    );
}