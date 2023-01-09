package com.example.bluetooth_kitkat;

import android.graphics.Color;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.LegendRenderer;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.util.ArrayList;

public class Graph extends AppCompatActivity {

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setContentView(R.layout.graph_activity);
            GraphView graph = (GraphView) findViewById(R.id.graph);

            String string = getIntent().getStringExtra("STRING"); // on recupere de l'autre activite la chaine de caractere
            ArrayList<Integer> list = toListe(string); // puis on extrait les vitesses a l'interieur pour les mettre dans une List d'entier

            LineGraphSeries<DataPoint> series = new LineGraphSeries<DataPoint>();
            for (int i = 0; i < list.size(); i++) {
                series.appendData(new DataPoint(i, list.get(i)), true, 90);
            }

            series.setColor(Color.rgb(255, 0, 0));
            series.setDrawDataPoints(true);
            series.setDataPointsRadius(5);
            series.setThickness(3);
            graph.getViewport().setScrollable(true); // enables horizontal scrolling
            graph.getViewport().setScalable(true);
            graph.getViewport().setScalableY(true);
            graph.addSeries(series);

            graph.setTitle("Speed");
            graph.setTitleTextSize(50);
            graph.setTitleColor(Color.WHITE);
            graph.getLegendRenderer().setVisible(true);
            graph.getLegendRenderer().setAlign(LegendRenderer.LegendAlign.TOP);

            GridLabelRenderer gridLabel = graph.getGridLabelRenderer();
            gridLabel.setHorizontalAxisTitle("X Axis Title");
            gridLabel.setVerticalAxisTitle("Y Axis Title");
        }

    ArrayList<Integer> toListe(String myString){

        ArrayList<Integer> list = new ArrayList<>();
        int i = 0;
        String buff = "";

        while(i<myString.length()){

            char c = myString.charAt(i);

            while(c !='/' && i<myString.length()-1) {
                buff+=c;
                i++;
                c = myString.charAt(i);
            }

            list.add(Integer.valueOf(buff));
            buff = "";

            if(c=='+'){
                return list;
            }

            i++;
        }

        return list;
    }
}
