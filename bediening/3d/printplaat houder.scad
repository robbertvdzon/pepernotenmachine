    difference() {
        union() {
            translate([-5,-5,0])
                cube([111.5+10, 98.3+10, 3]);
            translate([0,0,0]) rotate([0, 0, 0])  cylinder(h=11, d=10,  $fn=40);
            translate([111.5,0,0]) rotate([0, 0, 0])  cylinder(h=11, d=10,  $fn=40);
            translate([0,98.3,0]) rotate([0, 0, 0])  cylinder(h=11, d=10,  $fn=40);
            translate([111.5,98.3,0]) rotate([0, 0, 0])  cylinder(h=11, d=10,  $fn=40);

        }
        union() {
            translate([-5+10,-5+10,-1])
                cube([111.5-10, 98.3-10, 10]);

            translate([0,0,-1]) rotate([0, 0, 0])  cylinder(h=77, d=3.5,  $fn=40);
            translate([111.5,0,-1]) rotate([0, 0, 0])  cylinder(h=77, d=3.5,  $fn=40);
            translate([0,98.3,-1]) rotate([0, 0, 0])  cylinder(h=77, d=3.5,  $fn=40);
            translate([111.5,98.3,-1]) rotate([0, 0, 0])  cylinder(h=77, d=3.5,  $fn=40);
            
        }
    }
