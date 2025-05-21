include <jl_scad/box.scad>
include <jl_scad/parts.scad>
include <jl_scad/utils.scad>
$fs = $preview?0.5:0.125;
$fa = 1;

$slop = 0.08;
box_sz = [47.5,47.5,10];
$box_outside_color = "#fffb";
$box_inside_color = "#9a9b";
$box_cut_color = "#0000";

// halves=[BOT]
box_make(print=true)
box_shell_base_lid(box_sz,rsides=6,wall_sides=1.6,wall_top=2,base_height=3,rim_height=2,rbot_inside=1,rtop_inside=1)
{
    size = $parent_size;

    // usb c 
    translate([0,0,3])box_part(TOP+FRONT)  box_cutout(rect([12,6]),depth=2,anchor=CENTER);

    // bat cables
    translate([0,0,0])box_part(TOP+BACK)  box_cutout(rect([5,2]),depth=2);

    // display
    translate([-2,12,0])box_part(TOP)  box_cutout(rect([27,12]),depth=2,anchor=FRONT);
    // text
     box_part(TOP, inside=false)
         translate([0,-7,0])Z(-0.4) box_cut() text3d("Makerville", h=2, size=7, atype="ycenter", anchor=BOTTOM);
    // vents
    X(11) box_cut() //for(a = [BACK,FRONT])
        box_part(LEFT) xcopies(3,4,sp=0) cuboid([1.5,8,8],rounding=0.75,anchor=CENTER);

    // screw mounts
    for(a = [BACK+LEFT, FRONT+RIGHT, BACK+RIGHT, FRONT+LEFT])
        M(a * -0.5) position(a) box_screw_clamp(od=6.4,anchor=a,gap=2);
            
    // buttons
    translate([16,-13.5,0])box_part(TOP) box_cut() cyl(d=5,h=15,anchor=CENTER); 
    translate([-16,-13.5,0])box_part(TOP) box_cut() cyl(d=5,h=15,anchor=CENTER); 

}
