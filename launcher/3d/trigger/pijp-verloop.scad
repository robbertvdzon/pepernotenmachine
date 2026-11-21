module pijp1() {
difference() {
union() {

    translate([0, 0, 0])
      cylinder(h=20, r=71/2, $fn=100, center=false);

}
    translate([0, 0, -0.01])
      cylinder(h=20.1, r=68/2, $fn=100, center=false);
//
}
}

module pijp2() {
difference() {
union() {

    translate([0, 0, 0])
      cylinder(h=20, r=54/2, $fn=100, center=false);

}
    translate([0, 0, -0.01])
      cylinder(h=20.1, r=51/2, $fn=100, center=false);

}
}

module tussenstuk() {
difference() {
union() {

    translate([0, 0, 0])
      cylinder(h=20, r2=54/2, r1=71/2 ,$fn=100, center=false);

}
    translate([0, 0, -0.01])
      cylinder(h=20.1, r2=44/2,r1=68/2, $fn=100, center=false);

}
}



translate([0, 0, 0]) pijp1();
translate([0, 0, 20]) tussenstuk();
translate([0, 0, 40]) pijp2();
