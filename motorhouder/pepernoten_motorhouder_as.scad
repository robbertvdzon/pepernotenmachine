
difference(){
	union(){



        translate([0,0,0]){
            rotate([0,0,0]){
                cylinder(h=80, r=13, $fn=100, center=false);
            }
        }



}
	union() {
        translate([9,20,15]){
            rotate([120,10,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }

        translate([0,30,4]){
            rotate([90,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }

        translate([0,0,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=4.15, $fn=100, center=false);
            }
        }
   
    

      
	}
}
