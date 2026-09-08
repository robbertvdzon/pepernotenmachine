
difference(){
	union(){




      translate([-25,-30,0]){
            cube([50,40,4], center=false);
      }
      translate([-25,-34,0]){
            cube([50,4,20], center=false);
      }




}
	union() {

        translate([18.5,0,12]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([-18.5,0,12]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }

        translate([-18.5,-34+34,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([18.5,-34+34,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2.5, $fn=100, center=false);
            }
        }
        translate([0,-34+34,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=5, $fn=100, center=false);
            }
        }




    

      
	}
}
