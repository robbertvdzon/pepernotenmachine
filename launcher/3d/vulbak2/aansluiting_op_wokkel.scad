    difference() {
        
        
        union() {

            translate([0,0,-10]){
                cube([95, 104, 70], center=false);
            }
            translate([-5,-5,58]){
                cube([95+10, 104+10, 2], center=false);
            }

        }    

            translate([2,2,0]){
                cube([95-4, 100, 200], center=false);
            }

            translate([0, 52, 0])
            rotate([0, 90, 0]) 
                cylinder(h=1000, d=100, center=true, $fn=40);
            
            translate([0, 52, 0])
            rotate([0, 90, 0]) 
                cylinder(h=40, d=106, center=true, $fn=40);



    }

