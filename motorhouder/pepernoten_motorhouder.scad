
difference(){
	union(){




      translate([-30,-30,0]){
            cube([60,60,4], center=false);
      }
      translate([-30,-34,0]){
            cube([60,4,30], center=false);
      }
      translate([-34,-34,0]){
            cube([4,20,30], center=false);
      }
      translate([30,-34,0]){
            cube([4,20,30], center=false);
      }



}
	union() {

        translate([20,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-20,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }

        translate([-24,-24,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([-24,24,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([24,-24,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([24,24,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }




    

      
	}
}
