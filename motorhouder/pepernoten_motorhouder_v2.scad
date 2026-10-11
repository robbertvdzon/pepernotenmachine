
difference(){
	union(){




      translate([-47/2,-27,-2]){
            cube([47,55,6], center=false);
      }
      translate([-45/2-10,-27,-10]){
            cube([45+20,6,40], center=false);
      }
      translate([-47/2,-27,-10]){
            cube([4,25,40], center=false);
      }
      translate([47/2-4,-27,-10]){
            cube([4,25,40], center=false);
      }



}
	union() {

        translate([28,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-28,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([28,0,-5]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-28,0,-5]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }


        translate([-19/2,-19/2+10,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }
        translate([-19/2,19/2+10,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }
        translate([19/2,-19/2+10,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }
        translate([19/2,19/2+10,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=1.5, $fn=100, center=false);
            }
        }

        translate([0,10,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=5, $fn=100, center=false);
            }
        }

        translate([0,10,4-2.5]){
            rotate([0,0,0]){
                cylinder(h=20, r=11.5, $fn=100, center=false);
            }
        }


    

      
	}
}
