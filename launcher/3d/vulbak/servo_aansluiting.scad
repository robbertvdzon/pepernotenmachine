module pepernoten_servo_aansluiting() {
    difference() {
        
        
        union() {
            // Montageplaat
            translate([-65/2, -31/2, -4/2])
                cube([65, 31, 4]);

            
            translate([-65/2-5, -31/2, -2-5]){
                cube([75, 4.5, 20], center=false);
            }


        }    


        

        // Opening voor servo body
        translate([0, 0, 0])
            cube([42.5, 22, 10], center=true);

        // 4 montagegaten
        translate([-49.5/2, -10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([-49.5/2,  10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([ 49.5/2, -10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([ 49.5/2,  10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([-49.5/2, -10/2, 7])
        rotate([90, 0, 0]) 
            cylinder(h=1000, d=3.2, center=true, $fn=40);


        translate([49.5/2, -10/2, 7])
        rotate([90, 0, 0]) 
            cylinder(h=1000, d=3.2, center=true, $fn=40);

    }
}

pepernoten_servo_aansluiting();