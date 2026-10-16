module pepernoten_servo_aansluiting() {
    difference() {
        
        
        union() {

            translate([-65/2, -31/2, -(110-19-19)/2]){
                cube([65, 4.5, (110-19-19)/2], center=false);
            }

            translate([-65/2, -31/2-10, -(110-19-19+8)/2]){
                cube([65, 4.5+10, (110-19-19+8)/2], center=false);
            }


            translate([-65/2, -31/2, -(110-19-19)/2-4])
                cube([65, 31, 4]);


            

        }    

            translate([0, -68, 0])
            rotate([0, 90, 0]) 
                cylinder(h=1000, d=105, center=true, $fn=40);

        

        // Opening voor servo body
        translate([0, 0, 0])
            cube([42.5, 22, 1000], center=true);

        // 4 montagegaten
        translate([-49.5/2, -10/2, 0])
            cylinder(h=1000, d=3.2, center=true, $fn=40);

        translate([-49.5/2,  10/2, 0])
            cylinder(h=1000, d=3.2, center=true, $fn=40);

        translate([ 49.5/2, -10/2, 0])
            cylinder(h=1000, d=3.2, center=true, $fn=40);

        translate([ 49.5/2,  10/2, 0])
            cylinder(h=1000, d=3.2, center=true, $fn=40);



        translate([-29, -10/2, -5])
        rotate([90, 0, 0]) 
            cylinder(h=1000, d=3.2, center=true, $fn=40);

        translate([29, -10/2, -5])
        rotate([90, 0, 0]) 
            cylinder(h=1000, d=3.2, center=true, $fn=40);

        translate([-29, -10/2, -15])
        rotate([90, 0, 0]) 
            cylinder(h=1000, d=3.2, center=true, $fn=40);


        translate([29, -10/2, -15])
        rotate([90, 0, 0]) 
            cylinder(h=1000, d=3.2, center=true, $fn=40);


    }
}

pepernoten_servo_aansluiting();