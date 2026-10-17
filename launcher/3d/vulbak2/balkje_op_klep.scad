    difference() {
        
        
        union() {
            translate([35, -65, 0])
                cube([10, 130, 5]);
            translate([0, -65, 0])
                cube([45, 3, 5]);
            translate([0, 65-3, 0])
                cube([45, 3, 5]);
            translate([10, -40, 0])
                cube([25, 80, 3]);


            translate([0, -65, 0])
                cube([10, 3, 17]);
            translate([0, 65-3, 0])
                cube([10, 3, 17]);

            translate([5, 65, 17])
            rotate([90, 0, 0]) 
                cylinder(h=3, d=10, $fn=40);

            translate([5, -65+3, 17])
            rotate([90, 0, 0]) 
                cylinder(h=3, d=10, $fn=40);


        }    
            translate([5, 0, 17])
            rotate([90, 0, 0]) 
                cylinder(h=1000, d=3, center=true, $fn=40);
//
        

    }


