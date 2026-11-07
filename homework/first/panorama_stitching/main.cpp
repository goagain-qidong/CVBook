#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <random>
#include <algorithm>

using namespace std;
using namespace cv;
//计算A+
bool inverse(vector<Point2f>&src,vector<Point2f>&dst,Mat&H){
    int n=src.size();
    if(n<4||n!=dst.size()){
        return 0;
    }
    Mat A=Mat::zeros(2*n,8,CV_64F);
    Mat b=Mat::zeros(2*n,1,CV_64F);
    for(int i=0;i<n;i++){
        double x=src[i].x,y=src[i].y,u=dst[i].x,v=dst[i].y;
        int r1=2*i,r2=2*i+1;
        A.at<double>(r1,0)=x;
        A.at<double>(r1,1)=y;
        A.at<double>(r1,2)=1;
        A.at<double>(r1,6)=-u*x;
        A.at<double>(r1,7)=-u*y;
        A.at<double>(r2,3)=x;
        A.at<double>(r2,4)=y;
        A.at<double>(r2,5)=1;
        A.at<double>(r2,6)=-v*x;
        A.at<double>(r2,7)=-v*y;   
        b.at<double>(r1,0)=u;
        b.at<double>(r2,0)=v;     
    }
    Mat invA;
    double rcond=invert(A,invA,DECOMP_SVD);
    if(invA.empty()||rcond<=1e-12){
        return 0;
    }
    Mat h=invA*b;
    H=Mat::eye(3,3,CV_64F);
    H.at<double>(0,0)=h.at<double>(0,0);
    H.at<double>(0,1)=h.at<double>(1,0);
    H.at<double>(0,2)=h.at<double>(2,0);
    H.at<double>(1,0)=h.at<double>(3,0);
    H.at<double>(1,1)=h.at<double>(4,0);
    H.at<double>(1,2)=h.at<double>(5,0);
    H.at<double>(2,0)=h.at<double>(6,0);
    H.at<double>(2,1)=h.at<double>(7,0);
    return checkRange(H);

}
//随机取四对点
bool randomsample(vector<Point2f>&src,vector<Point2f>&dst,vector<Point2f>&sam1,vector<Point2f>&sam2,vector<int>&sel,mt19937&rng){
    if(src.size()<4||src.size()!=dst.size()){
        return 0;
    }
    sam1.clear();
    sam2.clear();
    sel.clear();
    uniform_int_distribution<int>dist(0,(int)src.size()-1);
    while(sel.size()<4){
        int idx=dist(rng);
        if(find(sel.begin(),sel.end(),idx)==sel.end()){
            sel.push_back(idx);
        }
    }
    for(int idx:sel){
        sam1.push_back(src[idx]);
        sam2.push_back(dst[idx]);
    }
    return 1;

}
//判断内点
bool findinliers(vector<Point2f>&src,vector<Point2f>&dst,Mat&H,double d,vector<int>&inliers){
    vector<Point2f>temp;
    perspectiveTransform(src,temp,H);
    inliers.clear();
    for(int i=0;i<src.size();i++){
        double err=norm(temp[i]-dst[i]);
        if(err<=d){
            inliers.push_back(i);
        }
    }
    return 1;
}
//ransac
bool ransac(vector<Point2f>&src,vector<Point2f>&dst,Mat&H,vector<int>&best,int cnt=3000,double d=3.0){
    int n=src.size();
    if(n<4||n!=dst.size()){
        return 0;
    }
    mt19937 rng(42);
    best.clear();
    for(int i=0;i<cnt;i++){
        vector<Point2f>sam1,sam2;
        vector<int>sel;
        if(!randomsample(src,dst,sam1,sam2,sel,rng)){
            continue;
        }
        Mat tempH;
        if(!inverse(sam1,sam2,tempH)){
            continue;
        }
        vector<int>cur;
        if(!findinliers(src,dst,tempH,d,cur)){
            continue;
        }
        if(cur.size()>best.size()){
            best=cur;
        }
    }
    if(best.size()<4){
        return 0;
    }
    vector<Point2f>bestsrc,bestdst;
    for(auto&id:best){
        bestsrc.push_back(src[id]);
        bestdst.push_back(dst[id]);
    }
    return inverse(bestsrc,bestdst,H);
}
Rect largestRec(Mat&mask){
    int rows=mask.rows;
    int cols=mask.cols;
    vector<int>hei(cols+1,0);
    long long best=0;
    Rect ans;
    for(int y=0;y<rows;y++){
        for(int x=0;x<cols;x++){
            uchar p=mask.at<uchar>(y,x);
            if(p==255){
                hei[x]++;
            }else{
                hei[x]=-1;
            }
        }
        vector<int>st;
        for(int x=0;x<=cols;x++){
            while(!st.empty()&&hei[st.back()]>hei[x]){
                int h=hei[st.back()];
                st.pop_back();
                int left=st.empty()?0:st.back()+1;
                int right=x-1;
                int wid=right-left+1;
                long long area=1LL*h*wid;
                if(area>best){
                    best=area;
                    int top=y-h+1;
                    ans=Rect(left,top,wid,h);
                }
            }
            st.push_back(x);
        }
    }
    return ans;
}
int main() {
    //读取图片
    Mat img1=imread("images/campus1.jpg",IMREAD_COLOR);
    Mat img2=imread("images/campus2.jpg",IMREAD_COLOR);
    if(img1.empty()||img2.empty()){
        cout<<"图片读取失败"<<endl;
        return -1;
    }
    cout<<"图片1："<<img1.cols<<"x"<<img1.rows<<endl;
    cout<<"图片2："<<img2.cols<<'x'<<img2.rows<<endl;
    //转灰度图
    Mat gray1,gray2;
    cvtColor(img1,gray1,COLOR_BGR2GRAY);
    cvtColor(img2,gray2,COLOR_BGR2GRAY);
    //提取特征点与描述子
    Ptr<ORB>orb =ORB::create(2000);
    vector<KeyPoint>kp1,kp2;
    Mat desc1,desc2;
    orb->detectAndCompute(gray1,noArray(),kp1,desc1);
    orb->detectAndCompute(gray2,noArray(),kp2,desc2);
    //特征点匹配
    BFMatcher matcher(NORM_HAMMING);
    vector<vector<DMatch>>knn;
    matcher.knnMatch(desc1,desc2,knn,2);
    vector<DMatch>good;
    float d=0.75f;
    for(auto&m:knn){
        if(m.size()<2)continue;
        if(m[0].distance<d*m[1].distance){
            good.push_back(m[0]);
        }
    }
    cout<<"初始匹配点对数量："<<good.size()<<endl;
    vector<Point2f>srcp,dstp;
    for(auto&m:good){
        Point2f p2=kp2[m.trainIdx].pt;
        Point2f p1=kp1[m.queryIdx].pt;
        srcp.push_back(p2);
        dstp.push_back(p1);
    }
    //计算H与内点
    Mat H;
    vector<int>best;
    if(ransac(srcp,dstp,H,best)){
        cout<<"内点对数量："<<best.size()<<endl;
        cout<<"外点对数量："<<srcp.size()-best.size()<<endl;
        cout<<"H："<<H<<endl;
    }else{
        cout<<"RANSAC计算失败"<<endl;
        return -1;
    }
    vector<DMatch>inlierMatchers;
    for(auto&id:best){
        inlierMatchers.push_back(good[id]);
    }
    //角点转换
    vector<Point2f>corner;
    corner.push_back(Point2f(0,0));
    corner.push_back(Point2f(img2.cols-1,0));
    corner.push_back(Point2f(img2.cols-1,img2.rows-1));
    corner.push_back(Point2f(0,img2.rows-1));
    vector<Point2f>trancor;
    perspectiveTransform(corner,trancor,H);
    for(int i=0;i<4;i++){
    cout<<"转换后角点："<<trancor[i]<<endl;
    }

    double minx=0;
    double miny=0;
    double maxx=img1.cols - 1;
    double maxy=img1.rows - 1;
    for(auto&p:trancor){
        minx=min(minx,(double)p.x);
        miny=min(miny,(double)p.y);
        maxx=max(maxx,(double)p.x);
        maxy=max(maxy,(double)p.y);
    }
    int wid=ceil(maxx)-floor(minx)+1;
    int hei=ceil(maxy)-floor(miny)+1;
    cout<<"画布宽度："<<wid<<"，画布高度："<<hei<<endl;
    //平移矩阵T
    Mat T=Mat::eye(3,3,CV_64F);
    T.at<double>(0,2)=-floor(minx);
    T.at<double>(1,2)=-floor(miny);
    //最终变换矩阵
    Mat finalH=T*H;
    cout<<"最终变换矩阵："<<finalH<<endl;
    //拼接,p=(1-a)*p1+a*p2
    Mat warp1,warp2;
    warpPerspective(img1,warp1,T,Size(wid,hei));
    warpPerspective(img2,warp2,finalH,Size(wid,hei));

    Mat panorama=Mat::zeros(hei,wid,CV_8UC3);
    Mat mask1(img1.size(),CV_8UC1,Scalar(255)),mask2(img2.size(),CV_8UC1,Scalar(255));
    Mat warpmask1,warpmask2;//0-不能复制，1-能复制
    warpPerspective(mask1,warpmask1,T,Size(wid,hei),INTER_NEAREST);
    warpPerspective(mask2,warpmask2,finalH,Size(wid,hei),INTER_NEAREST);
    vector<int>left(hei,wid),right(hei,-1);
    for(int y=0;y<hei;y++){
        for(int x=0;x<wid;x++){
            bool v1=warpmask1.at<uchar>(y,x)>0;
            bool v2=warpmask2.at<uchar>(y,x)>0;
            if(v1&&v2){
               left[y]=min(left[y],x);
               right[y]=max(right[y],x);
            }
        }
    }

    for(int y=0;y<hei;y++){
        for(int x=0;x<wid;x++){
            bool v1=warpmask1.at<uchar>(y,x)>0;
            bool v2=warpmask2.at<uchar>(y,x)>0;
            if(v1&&v2){
                int l=left[y],r=right[y];
                double a=0.5;
                if(r>l){
                    a=double(x-l)/(r-l);
                }
                Vec3b p1=warp1.at<Vec3b>(y,x),p2=warp2.at<Vec3b>(y,x);
                Vec3b& out=panorama.at<Vec3b>(y,x);
                for(int c=0;c<3;c++){
                    out[c]=saturate_cast<uchar>((1-a)*p1[c]+a*p2[c]);
                }
            }else if(v1){
                panorama.at<Vec3b>(y,x)=warp1.at<Vec3b>(y,x);
            }else if(v2){
                panorama.at<Vec3b>(y,x)=warp2.at<Vec3b>(y,x);
            }
        }
    }
    Mat validmask;
    bitwise_or(warpmask1,warpmask2,validmask);
    Mat binarymask;
    threshold(validmask,binarymask,127,255,THRESH_BINARY);
    Rect roi=largestRec(binarymask);
    Mat crop = panorama(roi).clone();
    
    


    Mat matchimage,matchimage2;
    drawMatches(img1,kp1,img2,kp2,inlierMatchers,matchimage2);
    drawMatches(img1,kp1,img2,kp2,good,matchimage);


    Mat show1,show2;
    drawKeypoints(img1,kp1,show1);
    drawKeypoints(img2,kp2,show2);



    filesystem::create_directories("outputs");
    imwrite("outputs/orb1.jpg",show1);
    imwrite("outputs/orb2.jpg",show2);
    imwrite("outputs/match.jpg",matchimage);
    imwrite("outputs/inliermatch.jpg",matchimage2);
    imwrite("outputs/warp1.jpg", warp1);
    imwrite("outputs/warp2.jpg", warp2);
    imwrite("outputs/panorama_alpha.jpg", panorama);
    imwrite("outputs/panorama_crop.jpg", crop);
    imwrite("outputs/valid_mask.jpg", validmask);

    waitKey(0);
    destroyAllWindows();

    return 0;
}
