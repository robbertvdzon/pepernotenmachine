
difference(){
	union(){


      translate([0,0,0]){
            cube([100,40,2], center=false);
      }


}
	union() {

        translate([10,10,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([10,30,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([90,10,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([90,30,-1]){
            rotate([0,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }



    

      
	}
}
