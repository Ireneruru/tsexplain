# Experiment in C 


## Setting of Experiment 

* run main.cpp : LIQUOR, SP500, COVID

* feature_hier: change explain features: 

* line 169:

        auto visual = visual_segmentation(trend_full, 0.1, 10);

    set the bound of visual segmentation. 

* line 200, if uncommented , only output visualization segmentation. 
Otherwise, output the explanation segmentation.

```
    // these two line is to assign the visualization segment
    //starts.clear();
    //for (auto& v : visual) starts.push_back(v.first);
```

* output files: 
    output.json: segmentation information for visualization 
    segment.pkl: all segments' explanation
    
* Visualization: 
    cd plot/ 
    python3 plot; open output.pdf