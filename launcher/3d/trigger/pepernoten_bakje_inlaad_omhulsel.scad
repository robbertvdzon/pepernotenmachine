module pepernoten_inlaad() {
difference() {
union() {



    translate([-40, -20, 57])
        cube([62, 26+40, 3]);
    translate([-40, -20, 0])
        cube([62, 2, 57]);
    translate([-40, 44, 0])
        cube([62, 2, 57]);
    translate([-40, -35, 0])
        cube([62, 15, 3]);
    translate([-40, 44, 0])
        cube([62, 15, 3]);

}

    translate([-31, -0.5, 0])
        cube([57, 27, 62]);


    translate([10,-28,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }
    translate([-10,-28,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }
    translate([-30,-28,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }


    translate([10,52,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }
    translate([-10,52,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }
    translate([-30,52,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=1.5, $fn=100, center=false);
        }
    }





}
}

pepernoten_inlaad();