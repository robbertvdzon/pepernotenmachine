// Pepernotenpaneel — buitenrand, displayuitsparing en boorgaten.
// Eenheden: mm. Onderaanzicht voorkomen: tekstzijde ligt boven (positieve Z).
// Oorsprong: linkeronderhoek van de buitencontour; X rechts, Y omhoog.
// Geometrie overgenomen uit het actuele Illustrator/SVG-document.
// Kleine afrondingsverschillen in de buitenmaten komen uit dat document.
plaat_breedte = 154.999972;
plaat_hoogte = 165.001222;
hoekradius = 10.001250;
plaat_dikte = 3; // Pas aan aan de gewenste plaatdikte.
led_diameter = 8;
schakelaar_diameter = 6;
drukknop_diameter = 12;
draaiknop_diameter = 7;
montage_diameter = 2;
display_breedte = 38.001222; // circa 38 mm
display_hoogte = 30.998583; // circa 31 mm
display_linksonder = [44.749861, 118.501583];
led_gaten = [
    [55.502528, 95.398167],
    [95.500472, 95.398167],
    [22.500167, 25.498778],
    [38.071778, 25.498778],
    [53.643389, 25.498778],
    [69.215000, 25.498778],
    [84.786611, 25.498778],
    [100.358222, 25.498778],
    [115.929833, 25.498778],
    [131.501444, 25.498778]
];
schakelaar_gaten = [
    [25.502306, 95.398167],
    [22.479000, 57.615667],
    [49.466500, 57.615667],
    [76.454000, 57.615667],
    [103.441500, 57.615667]
];
drukknop_gaten = [
    [22.500167, 133.999111],
    [131.501444, 95.398167],
    [130.432528, 57.615667]
];
draaiknop_gaten = [
    [99.500972+5, 133.999111]
];
montage_gaten = [
    [35.249556, 148.999222],
    [35.249556, 118.999000],
    [86.751583, 148.999222],
    [86.751583, 118.999000]
];
$fn = 96;
module buitenvorm_2d() {
    translate([hoekradius, hoekradius])
        offset(r = hoekradius)
            square([plaat_breedte - 2*hoekradius,
                    plaat_hoogte - 2*hoekradius]);
}
module paneel_2d() {
    difference() {
        buitenvorm_2d();
        translate(display_linksonder)
            square([display_breedte, display_hoogte]);
        for (punt = led_gaten)
            translate(punt) circle(d = led_diameter);
        for (punt = schakelaar_gaten)
            translate(punt) circle(d = schakelaar_diameter);
        for (punt = drukknop_gaten)
            translate(punt) circle(d = drukknop_diameter);
        for (punt = draaiknop_gaten)
            translate(punt) circle(d = draaiknop_diameter);
        for (punt = montage_gaten)
            translate(punt) circle(d = montage_diameter);
    }
}
// Voor een vlakke DXF/SVG-export: vervang onderstaande regel door paneel_2d();
linear_extrude(height = plaat_dikte) paneel_2d();
