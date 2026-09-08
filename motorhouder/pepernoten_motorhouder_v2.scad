
difference(){
	union(){




      translate([-45/2,-27,0]){
            cube([45,45,4], center=false);
      }
      translate([-45/2,-27,0]){
            cube([45,4,30], center=false);
      }
      translate([-45/2,-27,0]){
            cube([4,15,30], center=false);
      }
      translate([45/2-4,-27,0]){
            cube([4,15,30], center=false);
      }



}
	union() {

        translate([12,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-12,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }


        translate([-19/2,-19/2,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }
        translate([-19/2,19/2,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }
        translate([19/2,-19/2,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }
        translate([19/2,19/2,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }




    

      
	}
}
