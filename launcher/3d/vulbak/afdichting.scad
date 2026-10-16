
use <afdichting-mal.scad>;

difference() {
    union() {
        // Hoofdplaat
        translate([0, -104/2, 0]) {
            cube([3, 104, 200]);
        }

        // Aansluitpunt 1 - onderkant links
        translate([-30, -104/2, 0]) {
            cube([30, 7, 15]);
        }

        // Aansluitpunt 2 - onderkant rechts
        translate([-30, 104/2-7, 0]) {
            cube([30, 7, 15]);
        }

        // Aansluitpunt 3 - bovenkant
        translate([-100, -8, 170]) {
            cube([100, 16, 30]);
        }
    }

    // Schuine uitsparing
    rotate([0, 15, 0]) {
        translate([0, 0, -1]) {
            afdichting_mal();
        }
    }
}
