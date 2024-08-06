double regressPh(double x) {
  double terms[] = {
    -6.9166547464124113e+005,
    1.3125982761604455e+006,
    -7.3845177407151216e+005,
    -7.5630999639436137e+004,
    2.2883163010343912e+005,
    -8.3380483977886848e+004,
    9.8781468173595022e+003
  };

  double t = 1;
  double r = 0;
  for (double c : terms) {
    r += c * t;
    t *= x;
  }
  r = constrain(r, 3, 9);
  return r;
}