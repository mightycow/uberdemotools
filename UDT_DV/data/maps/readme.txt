This file describes the process to add a map to the 2D viewer. 

---=== Overview ===---

This process requires you to draw or render a visualization, and then derive the proper scaling values so that the entities read from the demo are placed correctly over your vis.

Given a map (ex: 'toxicity'), the 2D demo viewer expects two files:
- toxicity.png (2d vis)
- toxicity.txt (scaling data)

---=== 2d Visualization ===---

Create your 2D visualization of the map as you want. This is the part where you show your artistic skills :)

---=== Scalind Data ===---

Getting the scaling values is the tricky part. You basically have to take your visualization, and get what the coordinates of its corners are in Quake units. 

A dirty trick (used for the maps included in the release) is the following:

1) Open the map into Q3Radiant (or gtk, or any other radiant).
2) Go to the top view, zoom out a bit to leave some margin - the same margin you have in your vis - , make sure the grid is on, and then take a screenshot.
3) Go to photoshop and paste the screenshot on top of your 2D visualization.
4) Set the blending options so that you see both your vis and the radiant screenshot.
5) Scale the radiant screenshot so that the wirframe walls match those of your visualization.
6) Check the coordinates of the corners of your vis using the radiant grid. You need to get values for the top left corner (x1, y1), and bottom right corner (x2, y2).
7) Finally you need values for the height scaling. These are less crucial as they only govern the size of the player dots during the 2D demo view. In radiant you can just look at the z coordinate of the lowest floor (z1) and highest ceiling (z2).

Save your data into the txt file as:

------ map.txt starts ------
origin = x1 y1 z1
end = x2 y2 z2
------ map.txt ends ------
