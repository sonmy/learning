//
//  ZYRootViewController.swift
//  ZYSecurityPlayer
//
//  Created by mysong on 2017/5/3.
//  Copyright © 2017年 tencent. All rights reserved.
//

import UIKit
import Foundation
import CoreGraphics

//将 C 的 xxx 方法映射为 Swift 的 c_xxx 方法
@_silgen_name("zyavdecode") func c_zyavdecode(a: Int32) -> Int32

class ZYRootViewController: UIViewController {

    var testBtn: UIButton!
    var port: Int = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
        //let ret = c_zyavdecode(a: 31)
        //print(ret);
        //self.view.backgroundColor = UIColor.red
        
        testBtn = UIButton(frame: CGRect.init(x: 20, y: 40, width: 80, height: 32))
        testBtn.setTitle("Test", for: UIControlState.normal)
        testBtn.addTarget(self, action: #selector(testAction), for: UIControlEvents.touchUpInside)
        
        self.view.addSubview(testBtn)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    func testAction(sender: UIButton!) {
        
        let urlString = String.init(format: "http://127.0.0.1:%d/getMp4", self.port)
        
        let httpUrl = URL(string: urlString);
        let urlRequest = URLRequest(url: httpUrl!)
       
        let session = URLSession.shared
        
        let task = session.dataTask(with: urlRequest) { (data, response, error) in
            if (data != nil) {
                
                let str:String = String.init(data: data!, encoding: .utf8)!
                
                print("%s", str);
            }
            
            print("");
        };
        task.resume()
    }

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
    }
    */

}
