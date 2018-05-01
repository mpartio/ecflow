//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLOADVIEW_HPP
#define SERVERLOADVIEW_HPP

#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <QtCharts>
using namespace QtCharts;

#include <vector>

class LogLoadDataItem
{
public:
    LogLoadDataItem(const std::string& name) : sumTotal_(0), maxTotal_(0), rank_(-1), percentage_(0.), name_(name) {}
    LogLoadDataItem() : sumTotal_(0), maxTotal_(0), rank_(-1), percentage_(0.) {}

    void clear();
    void init(size_t num);

    size_t size() const {return childReq_.size();}
    float percentage() const {return percentage_;}
    void setPercentage(float v) {percentage_=v;}
    size_t sumTotal() const {return sumTotal_;}
    size_t maxTotal() const {return maxTotal_;}
    int rank() const {return rank_;}
    void setRank(int v) {rank_=v;}
    const std::vector<int>& childReq() const {return childReq_;}
    const std::vector<int>& userReq() const {return userReq_;}
    const std::string& name() const {return name_;}

    void add(size_t childVal,size_t userVal)
    {
        childReq_.push_back(static_cast<int>(childVal));
        userReq_.push_back(static_cast<int>(userVal));

        size_t tot=childVal + userVal;
        sumTotal_+=tot;
        if(maxTotal_ < tot)
            maxTotal_ = tot;
    }

protected:
    std::vector<int> childReq_;
    std::vector<int> userReq_;
    size_t sumTotal_;
    size_t maxTotal_;
    int rank_;
    float percentage_;
    std::string name_;
};

class LogLoadData
{
public:
    LogLoadData() : timeRes_(SecondResolution)  {}

    QStringList suiteNames() const {return suites_;}
    const std::vector<LogLoadDataItem>& suites() const {return suiteData_;}

    enum TimeRes {SecondResolution, MinuteResolution};
    void setTimeRes(TimeRes);

    void loadLogFile(const std::string& logFile);

    void getChildReq(QLineSeries& series);
    void getUserReq(QLineSeries& series);
    void getTotalReq(QLineSeries& series,int& maxVal);
    void getSuiteReq(QString suiteName,QLineSeries& series);

private:
    struct SuiteLoad {
       SuiteLoad(const std::string& name) : name_(name),
           childReq_(0),userReq_(0)  {}

       std::string name_;
       size_t   childReq_;
       size_t   userReq_;
    };

    void clear();
    void getSeries(QLineSeries& series,const std::vector<int>& vals);
    void getSeries(QLineSeries& series,const std::vector<int>& vals1,const std::vector<int>& vals2);
    void add(std::vector<std::string> time_stamp,size_t child_requests_per_second,
             size_t user_request_per_second,std::vector<SuiteLoad>& suite_vec);

    void processSuites();

    bool extract_suite_path(const std::string& line,bool child_cmd,std::vector<SuiteLoad>& suite_vec,
                            size_t& column_index);

    TimeRes timeRes_;
    std::vector<qint64> time_;
    LogLoadDataItem data_;
    std::vector<LogLoadDataItem> suiteData_;

    QStringList suites_;
};

class ChartView : public QChartView
{
    Q_OBJECT
public:
    ChartView(QChart *chart, QWidget *parent);

    void doZoom(QRectF);

Q_SIGNALS:
    void chartZoomed(QRectF);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void adjustTimeAxis(qint64 periodInMs);
};

class ServerLoadView : public QWidget
{
    Q_OBJECT
public:
    explicit ServerLoadView(QWidget* parent=0);

    void setData(LogLoadData*);

    void load(const std::string& logFile);
    void setResolution(LogLoadData::TimeRes);

protected Q_SLOTS:
    void slotZoom(QRectF);

protected:
    void load();
    void build(QChart* chart,QLineSeries *series,int maxVal);

    QChart* chart_;
    ChartView* chartView_;
    QChart* chartUserReq_;
    QChart* chartChildReq_;
    QList<ChartView*> views_;

    LogLoadData* data_;
};

#endif // SERVERLOADVIEW_HPP
