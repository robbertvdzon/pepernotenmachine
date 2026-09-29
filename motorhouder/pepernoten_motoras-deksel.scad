
difference(){
	union(){


      translate([0,0,0]){
            cube([20,57.5,4], center=false);
      }
      translate([0,0,0]){
            cube([20,4,40], center=false);
      }
      translate([0,53.5,-20]){
            cube([20,4,20], center=false);
      }




}
	union() {

        translate([6,100,-10]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }
        translate([14,100,-10]){
            rotate([90,0,0]){
                cylinder(h=500, r=2, $fn=100, center=false);
            }
        }




    

      
	}
}
