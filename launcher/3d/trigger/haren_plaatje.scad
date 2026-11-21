difference() {
union() {

    translate([0, 0, 0])
        cube([60, 20, 3]);

}

    translate([10,10,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }
    translate([50,10,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }





}

