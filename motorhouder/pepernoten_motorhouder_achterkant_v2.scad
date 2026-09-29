
difference(){
	union(){




      translate([-51/2,-27,0]){
            cube([51,35,4], center=false);
      }
      translate([-45/2-13,-27,-10]){
            cube([71,4,40], center=false);
      }
      translate([-45/2-13,-27,-10]){
            cube([10,6,40], center=false);
      }
      translate([45/2+3,-27,-10]){
            cube([10,6,40], center=false);
      }
      translate([-51/2,-27,-10]){
            cube([7,15,40], center=false);
      }
      translate([45/2-4,-27,-10]){
            cube([7,15,40], center=false);
      }



}
	union() {

        translate([-45/2-13+5,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([45/2+3+5,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-45/2-13+5,0,0]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([45/2+3+5,0,0]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }


        translate([-35/2,0,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=3, $fn=100, center=false);
            }
        }
        translate([35/2,0,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=3, $fn=100, center=false);
            }
        }
        translate([0,0,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=5, $fn=100, center=false);
            }
        }




    

      
	}
}
