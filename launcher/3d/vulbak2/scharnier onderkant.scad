    difference() {
        
        
        union() {
            // Montageplaat
            translate([-20/2, -60/2, 40])
                cube([20, 60, 14]);



        }    
            translate([0, 0, 0])
            rotate([0, 90, 0]) 
                cylinder(h=1000, d=105, center=true, $fn=40);
            translate([0, 21, 0])
            rotate([0, 0, 0]) 
                cylinder(h=1000, d=2, center=true, $fn=40);
            translate([0, -21, 0])
            rotate([0, 0, 0]) 
                cylinder(h=1000, d=2, center=true, $fn=40);


        

    }


