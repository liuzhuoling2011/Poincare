#ifndef __BSS_PNL_DEFINE_H__
#define __BSS_PNL_DEFINE_H__

#include "task_define.h"

#define BSS_MSG_SIZE 128

#pragma pack (8)

struct bss_order_info
{
	int       place_cnt;  //�µ�����
	int       trade_cnt;  //�ɽ�����
	double    total_amt;  //�ɽ���
	int       total_vol;  //�ɽ���
};

struct bss_position
{ 
	int       long_volume; //ʣ���λ��
	int       short_volume;
	double    long_price;
	double    short_price;
	double    today_cost;
};

struct bss_pnl_info
{
	int       strat_id;    //����id
	int       param_index;
	char      strat_name[MIN_BUFF_SIZE];
	int       max_vol;     //�������
	int       cancel_cnt;  //������
	double    principal;   //���ױ���
	double    rounds;      //�ܻغ���

	double    fee_vol;    //������
	double    gross_pnl;  //ë����
	double    net_pnl;    //������

	double    parameter[3]; //��ǰ����ֵ
	double    max_live_pnl; //�������
	double    max_tick_dd;  //���س�

	bss_order_info  order[3];  //��-0, ��-1�� ��-2
	bss_position    remain_pos;  //ʣ���λ
};

struct bss_day_node{
	int            trade_date;           //��������
	int            day_night;            //��ҹ����Ϣ
	int            pnl_cnt;              //ͳ����Ϣ�ĸ���
	double         total_pnl;            //����������
	double         earnings;             //����������
	bss_pnl_info **pnl_nodes;            //ͳ����Ϣ
};


struct  bss_task_ack 
{
	int    task_type;     //��������
	int    status;        //����״̬��
	char   message[BSS_MSG_SIZE];  //������Ϣ

	char   exchange;
	char   product[MAX_NAME_SIZE];
	char   symbol[MIN_BUFF_SIZE];
	int            day_cnt;   //���������
	bss_day_node  *day_nodes;  //ÿ������ݽ��
};

#pragma pack ()

#endif

