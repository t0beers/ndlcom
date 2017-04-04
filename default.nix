{ stdenv
, cmake
, package
}:

stdenv.mkDerivation rec {

  name = package.name;
  src = package.src;

  nativeBuildInputs = [ cmake ];
  enableParallelBuilding = true;
  hardeningDisable = [ "all" ];
}
