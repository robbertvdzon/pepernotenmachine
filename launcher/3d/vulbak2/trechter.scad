
$fn = 150;

// Afmetingen in mm
diameter_boven = 110;
diameter_onder = 75;

hoogte = 100;
wanddikte = 3;

// Trechter
module trechter() {
    difference() {
        // Buitenkant
        cylinder(
            h = hoogte,
            d1 = diameter_onder + 2*wanddikte,
            d2 = diameter_boven + 2*wanddikte
        );

        // Binnenkant
        translate([0, 0, -0.1])
            cylinder(
                h = hoogte + 0.2,
                d1 = diameter_onder,
                d2 = diameter_boven
            );
    }
}

trechter();
