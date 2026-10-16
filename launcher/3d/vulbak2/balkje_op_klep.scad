    difference() {
        
        
        union() {
            translate([-10/2, -110/2, 0])
                cube([10, 110, 3]);
            translate([-10/2, -110/2, 0])
                cube([10, 3, 10]);
            translate([-10/2, 110/2-3, 0])
                cube([10, 3, 10]);



        }    
            translate([0, 21, 0])
            rotate([0, 0, 0]) 
                cylinder(h=1000, d=2, center=true, $fn=40);
            translate([0, -21, 0])
            rotate([0, 0, 0]) 
                cylinder(h=1000, d=2, center=true, $fn=40);

            translate([0, 0, 7])
            rotate([90, 0, 0]) 
                cylinder(h=1000, d=2, center=true, $fn=40);

        

    }


