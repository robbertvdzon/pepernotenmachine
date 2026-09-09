
difference(){
	union(){



        translate([0,0,0]){
            rotate([0,0,0]){
                cylinder(h=4, r=26, $fn=100, center=false);
            }
        }



}
	union() {
        translate([0,0,-1]){
            rotate([0,0,0]){
                cylinder(h=50, r=20, $fn=100, center=false);
            }
        }
    

      
	}
}
