
difference(){
	union(){




      translate([-36,-30,-2]){
            cube([72,60,6], center=false);
      }
      translate([-34-12,-34,-10]){
            cube([68+24,4,40], center=false);
      }
      translate([-34-12,-34,-10]){
            cube([10,6,40], center=false);
      }
      translate([-34-12+82,-34,-10]){
            cube([10,6,40], center=false);
      }


      translate([-36,-34,-10]){
            cube([6,30,40], center=false);
      }
      translate([30,-34,-10]){
            cube([6,30,40], center=false);
      }



}
	union() {

        translate([41,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-41,0,20]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([41,02,0]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-41,0,0]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }

        translate([-24,-24,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([-24,24,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([24,-24,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([24,24,-10]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }




    

      
	}
}
