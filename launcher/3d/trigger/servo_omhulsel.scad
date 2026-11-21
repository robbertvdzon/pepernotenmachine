module pepernoten_inlaad() {
difference() {
union() {




    translate([-40, -20, 77])
        cube([100, 26+40, 2]);
    translate([-40, -20, 0])
        cube([100, 2, 77]);
    translate([-40, 44, 0])
        cube([100, 2, 77]);
    translate([-40, -30, 0])
        cube([100, 10, 2]);
    translate([-40, 44, 0])
        cube([100, 10, 2]);

}
    translate([-90, -50, 55])
        cube([100, 200, 30]);


    translate([-20,-22,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=2, $fn=100, center=false);
        }
    }
    translate([40,-22,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=2, $fn=100, center=false);
        }
    }


    translate([-20,50,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=2, $fn=100, center=false);
        }
    }
    translate([40,50,-1]){
        rotate([0,0,0]){
            cylinder(h=500, r=2, $fn=100, center=false);
        }
    }





}
}

pepernoten_inlaad();