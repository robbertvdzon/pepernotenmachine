
difference() {
union() {

    translate([0-5, 0, -10])
        cube([60+2, 10, 20]);

    translate([50+2, 0, -15])
        cube([5, 10, 30]);

    // Driehoekige uitsparing
    translate([-6-5, 5, -5])
        rotate([90, 0, 0])
            linear_extrude(height=10, center=true)
                polygon([
                    [0, 0],
                    [10, 0],
                    [6, 10]
                ]);


    
}



    translate([30-8, -5, -5-2.5])
        cube([10+8, 24, 15]);


}