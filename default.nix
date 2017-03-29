{ stdenv
, cmake
, actualSrc ? ./.
, ...
}:

stdenv.mkDerivation rec {

  name = "ndlcom";

  src = actualSrc;

  nativeBuildInputs = [ cmake ];
  enableParallelBuilding = true;
  hardeningDisable = [ "all" ];
}
