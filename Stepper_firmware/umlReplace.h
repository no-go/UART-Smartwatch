inline void umlReplace(char * inChar) {
  if (*inChar == -97) {
    *inChar = 224; // ß
  } else if (*inChar == -80) {
    *inChar = 248; // °
  } else if (*inChar == -67) {
    *inChar = 171; // 1/2
  } else if (*inChar == -78) {
    *inChar = 253; // ²
  } else if (*inChar == -92) {
    *inChar = 132; // ä
  } else if (*inChar == -74) {
    *inChar = 148; // ö
  } else if (*inChar == -68) {
    *inChar = 129; // ü
  } else if (*inChar == -124) {
    *inChar = 142; // Ä
  } else if (*inChar == -106) {
    *inChar = 153; // Ö
  } else if (*inChar == -100) {
    *inChar = 154; // Ü
  } else if (*inChar == -85) {
    *inChar = 0xAE; // <<
  } else if (*inChar == -69) {
    *inChar = 0xAF; // >>
  }
}
