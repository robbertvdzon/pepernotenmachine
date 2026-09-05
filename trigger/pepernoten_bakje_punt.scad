size = 20;          // breedte/lengte van het vierkant
straight_h = 50;    // rechte blok: 1 cm
point_h = 20;       // taps gedeelte: 1 cm




difference() {
union() {

    translate([-size/2-4, -size/2, point_h+straight_h+0])
        cube([size+8, size, 12]);

    translate([-size/2-20, -size/2, point_h+straight_h+12])
        cube([size+40, size, 5]);

    translate([-size/2-20, -size/2, point_h+straight_h+12])
        cube([size+40, size+12, 5]);


    translate([-size/2, -size/2, point_h])
        cube([size, size, straight_h]);

    // Vierkant dat naar een punt loopt
    hull() {
        // onderkant van het vierkant
        translate([-size/2, -size/2, point_h])
            cube([size, size, 0.01]);
        // punt onderaan
        translate([0, 0, 0])
            cube([0.01, 0.01, 0.01], center=true);
    }
    

}
    translate([2.5, -20, 20.1])
        cube([10, 40, 10]);

    // Driehoekige uitsparing
    translate([2.5, -0, 30])
        rotate([90, 0, 0])
            linear_extrude(height=32, center=true)
                polygon([
                    [0, 0],
                    [40, 0],
                    [15, 20]
                ]);

   translate([23,0,-200]){
      rotate([0,0,0]){
                    cylinder(h=1000, r=2.5, $fn=100, center=false);
      }
  }
 translate([-23,0,-200]){
      rotate([0,0,0]){
                    cylinder(h=1000, r=2.5, $fn=100, center=false);
      }
  }
 translate([0,0,-200]){
      rotate([0,0,0]){
          cylinder(h=1000, r=2.5, $fn=100, center=false);
      }
  }

}