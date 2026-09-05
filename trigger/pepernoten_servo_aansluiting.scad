module pepernoten_servo_aansluiting() {
    difference() {
        
        
        union() {
            // Montageplaat
            translate([-65/2-15, -31/2, -4/2])
                cube([65+30, 31+5.5, 4]);

            
            translate([-65/2, 0, -2]){
                cube([65, 26-10, 4], center=false);
            }
            //translate([-65/2, 22-10+4, -2]){
            //    cube([35, 15, 8], center=false);
            //}


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


        // gaten voor op ander stuk
        translate([ -49.5/2-15, -10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([ -49.5/2-15,  10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([ 49.5/2+15, -10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);

        translate([ 49.5/2+15,  10/2, 0])
            cylinder(h=10, d=3.2, center=true, $fn=40);
        

    }
}

pepernoten_servo_aansluiting();